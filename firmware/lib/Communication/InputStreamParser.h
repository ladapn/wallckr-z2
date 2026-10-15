#ifndef INPUT_STREAM_PARSER_H
#define INPUT_STREAM_PARSER_H

class IRobotIOStream;

enum class JoystickCommand {
  NO_COMMAND = 0,
  UP,
  RIGHT,
  DOWN,
  LEFT,
  UP_TRIANGLE,
  RIGHT_CIRCLE,
  DOWN_X,
  LEFT_SQUARE
};

/**
 * Interface for decoding information coming from user to robot
 */
class InputStreamParser {
  const int MIN_ALLOWED_CHAR = 'a';
  const int MAX_ALLOWED_CHAR = 'h';

public:
  virtual ~InputStreamParser() = default;

  /**
   * Decode data coming from user to robot
   */
  virtual JoystickCommand decode(IRobotIOStream &input_stream) = 0;

protected:
  bool is_allowed_cmd_char(char in) {
    if ((in >= MIN_ALLOWED_CHAR) && (in <= MAX_ALLOWED_CHAR)) {
      return true;
    }

    return false;
  }

  /**
   * Translate incoming data from character representation to JoystickCommand
   * universal representation
   */
  JoystickCommand input_to_joystick_command(char in) {
    JoystickCommand command = JoystickCommand::NO_COMMAND;

    switch (in) {
    case 'a':
      command = JoystickCommand::UP;
      break;
    case 'b':
      command = JoystickCommand::RIGHT;
      break;
    case 'c':
      command = JoystickCommand::DOWN;
      break;
    case 'd':
      command = JoystickCommand::LEFT;
      break;
    case 'e':
      command = JoystickCommand::UP_TRIANGLE;
      break;
    case 'f':
      command = JoystickCommand::RIGHT_CIRCLE;
      break;
    case 'g':
      command = JoystickCommand::DOWN_X;
      break;
    case 'h':
      command = JoystickCommand::LEFT_SQUARE;
      break;
    }

    return command;
  };
};

#endif // INPUT_STREAM_PARSER_H
