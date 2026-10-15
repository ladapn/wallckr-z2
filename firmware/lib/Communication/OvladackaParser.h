#ifndef OVLADACKA_PARSER_H
#define OVLADACKA_PARSER_H

#include "IRobotIOStream.h"
#include "InputStreamParser.h"

enum class OvladackaParserState {
  WAITING_FOR_START = 0,
  WAITING_FOR_CMD,
  WAITING_FOR_ACK
};

/**
 * Implementation of communication protocol used by ovladacka tool
 */
class OvladackaParser : public InputStreamParser {
  OvladackaParserState state = OvladackaParserState::WAITING_FOR_START;
  const char START_CHAR = ':';
  const char ACK_CHAR = '!';
  char cmd_char = 0;

public:
  /**
   * Decode input format into JoysticCommand format
   * @param input_stream stream to receive input data from
   * @return decoded command in JoysticCommand format, or
   * JoystickCommand::NO_COMMAND if no command was present
   */
  JoystickCommand decode(IRobotIOStream &input_stream) override {
    JoystickCommand command = JoystickCommand::NO_COMMAND;
    int chars_processed = 0;
    const int MAX_CHARS_PER_ITER = 10;

    /* Limit number of characters to be processed in a row to prevent situation
     * when a long stream of incoming characters causes this function to block
     * others. */
    while (input_stream.available() && chars_processed++ < MAX_CHARS_PER_ITER) {
      auto current = input_stream.read();

      switch (state) {
      case OvladackaParserState::WAITING_FOR_START:
        if (current == START_CHAR) {
          state = OvladackaParserState::WAITING_FOR_CMD;
        }
        break;
      case OvladackaParserState::WAITING_FOR_CMD:
        if (is_allowed_cmd_char(current)) {
          state = OvladackaParserState::WAITING_FOR_ACK;
          cmd_char = current;
        } else {
          state = OvladackaParserState::WAITING_FOR_START;
        }
        break;
      case OvladackaParserState::WAITING_FOR_ACK:
        if (current == ACK_CHAR) {
          command = input_to_joystick_command(cmd_char);
        }

        state = OvladackaParserState::WAITING_FOR_START;
        break;
      }
    }

    return command;
  };
};

#endif
