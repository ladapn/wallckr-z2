#!/usr/bin/env python3
"""Run clang-tidy on our sources using a Zephyr build's compile_commands.json.

Two adjustments are needed before clang-tidy can use the database of a real
(GCC, arm-zephyr-eabi) Zephyr build:

- GCC-only flags that clang rejects as unknown arguments are dropped.
- Include paths outside firmware/ (Zephyr, west modules, generated headers)
  become -isystem, so warnings raised inside Zephyr's LOG_*/SHELL_*/DT_*
  macro expansions are not reported against our code.

Usage: clang_tidy.py <build-dir> <source>...
"""

import json
import re
import subprocess
import sys
from pathlib import Path

GCC_ONLY_FLAGS = {"-mfp16-format=ieee", "-fno-reorder-functions"}
FIRMWARE_DIR = (Path(__file__).resolve().parent.parent / "firmware").as_posix()


def adjust(command: str) -> str:
    args = [arg for arg in command.split() if arg not in GCC_ONLY_FLAGS]
    return " ".join(
        re.sub(r"^-I(?!" + re.escape(FIRMWARE_DIR) + ")", "-isystem", arg) for arg in args
    )


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2

    build_dir = Path(sys.argv[1])
    sources = sys.argv[2:]

    database = json.loads((build_dir / "compile_commands.json").read_text())
    for entry in database:
        entry["command"] = adjust(entry["command"])

    tidy_dir = build_dir / "clang-tidy"
    tidy_dir.mkdir(exist_ok=True)
    (tidy_dir / "compile_commands.json").write_text(json.dumps(database))

    return subprocess.call(
        ["clang-tidy", "--quiet", "-p", str(tidy_dir), "--extra-arg=--target=arm-none-eabi", *sources]
    )


if __name__ == "__main__":
    sys.exit(main())
