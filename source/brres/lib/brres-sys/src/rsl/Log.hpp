#pragma once

#include <fmt/format.h>
#include <string_view>

namespace rsl {

namespace logging {

enum class Level {
  Error,
  Warn,
  Info,
  Debug,
  Trace,
};

void init();
void debug(std::string_view s);
void error(std::string_view s);
void info(std::string_view s);
void log(Level level, std::string_view s);
void trace(std::string_view s);
void warn(std::string_view s);

template <typename... T>
inline void debug(fmt::format_string<T...> s, T&&... args) {
  auto buf = fmt::format(s, std::forward<T>(args)...);
  debug(buf);
}
template <typename... T>
inline void error(fmt::format_string<T...> s, T&&... args) {
  auto buf = fmt::format(s, std::forward<T>(args)...);
  error(buf);
}
template <typename... T>
inline void info(fmt::format_string<T...> s, T&&... args) {
  auto buf = fmt::format(s, std::forward<T>(args)...);
  info(buf);
}
template <typename... T>
inline void log(Level level, fmt::format_string<T...> s, T&&... args) {
  auto buf = fmt::format(s, std::forward<T>(args)...);
  log(level, buf);
}
template <typename... T>
inline void trace(fmt::format_string<T...> s, T&&... args) {
  auto buf = fmt::format(s, std::forward<T>(args)...);
  trace(buf);
}
template <typename... T> inline void warn(fmt::format_string<T...> s, T&&... args) {
  auto buf = fmt::format(s, std::forward<T>(args)...);
  warn(buf);
}

} // namespace logging

using namespace logging;

} // namespace rsl
