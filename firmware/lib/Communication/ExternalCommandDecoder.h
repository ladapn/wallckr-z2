#ifndef EXTERNAL_COMMAND_DECODER_H
#define EXTERNAL_COMMAND_DECODER_H

class IRobotIOStream;
class InputStreamParser;
struct RobotCommand;

/**
 * Class to decode input binary stream into command in RobotCommand struct
 * format
 */
class ExternalCommandDecoder {
  IRobotIOStream &cmd_stream;
  InputStreamParser &input_decoder;

public:
  /**
   * Constructor method
   * @param[in] cmdSerial stream used as a source for incoming commands
   */
  explicit ExternalCommandDecoder(IRobotIOStream &in_stream,
                                  InputStreamParser &in_parser)
      : cmd_stream(in_stream), input_decoder(in_parser) {};
  /**
   * Check if external command is present, if so, parse it and update impacted
   * field of robot_command structure
   * @param[out] robot_command structure to hold extracted robot command
   * (desired speed, steering angle etc)
   * @return true if there was a command and thus if any of the fields were
   * updated, false otherwise
   */
  bool check_external_command(RobotCommand &robot_command);
};
#endif // EXTERNAL_COMMAND_DECODER_H
