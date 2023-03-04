#include "Cli.hpp"

extern "C" int rs_parse_args(int argc, const char* const* argv,
                             CliOptions* args);

std::optional<CliOptions> parse(int argc, const char** argv) {
  CliOptions args{};
  int result = rs_parse_args(argc, argv, &args);

  if (result == 0) {
    return args;
  }
  return std::nullopt;
}
