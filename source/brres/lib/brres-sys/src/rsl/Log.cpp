#include "Log.hpp"

#include <core/common.h>

namespace rsl {
namespace logging {

extern "C" void rsl_log_init();
extern "C" void rsl_c_debug(const char* s, u32 len);
extern "C" void rsl_c_error(const char* s, u32 len);
extern "C" void rsl_c_info(const char* s, u32 len);
extern "C" void rsl_c_trace(const char* s, u32 len);
extern "C" void rsl_c_warn(const char* s, u32 len);

void init() { rsl_log_init(); }
void debug(std::string_view s_) {
  // TODO: Since rust API doesnt care about length, null terminate
  std::string s(s_);
  rsl_c_debug(s.c_str(), s.size());
}
void error(std::string_view s_) {
  // TODO: Since rust API doesnt care about length, null terminate
  std::string s(s_);
  rsl_c_error(s.c_str(), s.size());
}
void info(std::string_view s_) {
  // TODO: Since rust API doesnt care about length, null terminate
  std::string s(s_);
  rsl_c_info(s.c_str(), s.size());
}
void log(Level l, std::string_view s_) {
  // TODO: Since rust API doesnt care about length, null terminate
  std::string s(s_);
  switch (l) {
  case Level::Error:
    rsl_c_error(s.c_str(), s.size());
    break;
  case Level::Warn:
    rsl_c_warn(s.c_str(), s.size());
    break;
  case Level::Info:
    rsl_c_info(s.c_str(), s.size());
    break;
  case Level::Debug:
    rsl_c_debug(s.c_str(), s.size());
    break;
  case Level::Trace:
    rsl_c_trace(s.c_str(), s.size());
    break;
  }
}
void trace(std::string_view s_) {
  // TODO: Since rust API doesnt care about length, null terminate
  std::string s(s_);
  rsl_c_trace(s.c_str(), s.size());
}
void warn(std::string_view s_) {
  // TODO: Since rust API doesnt care about length, null terminate
  std::string s(s_);
  rsl_c_warn(s.c_str(), s.size());
}

} // namespace logging
} // namespace rsl
