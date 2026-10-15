#ifndef BLEJOYSTICK_DECODER_H
#define BLEJOYSTICK_DECODER_H

#include "IRobotIOStream.h"
#include "InputStreamParser.h"
#include <ctype.h>

enum class JoystickDecoderState {
  WAITING_FOR_UPPER = 0,
  WAITING_FOR_LOWER,
  WAITING_FOR_ZERO
};

/**
 * Implementation of communication protocol used by BLEJoystick application,
 * which sends commands in format "Aa\0" -> Upper case, lower case,
 * zero for each button press -> buttons are labeled from A to H
 */
class BLEJoystickDecoder : public InputStreamParser {
  JoystickDecoderState state = JoystickDecoderState::WAITING_FOR_UPPER;
  int prev = 0;
  int chars_processed = 0;
  const int MAX_CHARS_PER_ITER = 10;

public:
  /**
   * Decode input format into JoystickCommand format
   * @param input_stream stream to receive input data from
   * @return decoded command in JoystickCommand format, or
   * JoystickCommand::NO_COMMAND if no command was present
   */
  JoystickCommand decode(IRobotIOStream &input_stream) override {
    int current;
    const int DIFF_LOWER_UPPER = 'a' - 'A';

    while (input_stream.available() && chars_processed++ < MAX_CHARS_PER_ITER) {

      current = input_stream.read();

      switch (state) {
      case JoystickDecoderState::WAITING_FOR_UPPER:
        if (isupper(current)) {
          state = JoystickDecoderState::WAITING_FOR_LOWER;
          prev = current;
        }
        break;
      case JoystickDecoderState::WAITING_FOR_LOWER:
        if (islower(current) && current - prev == DIFF_LOWER_UPPER) {
          state = JoystickDecoderState::WAITING_FOR_ZERO;
          prev = current;
        } else {
          state = JoystickDecoderState::WAITING_FOR_UPPER;
        }
        break;
      case JoystickDecoderState::WAITING_FOR_ZERO:
        state = JoystickDecoderState::WAITING_FOR_UPPER;
        if (current == '\0') {
          return input_to_joystick_command(prev);
        }

        break;
      }
    }

    return JoystickCommand::NO_COMMAND;
  }
};

#endif
