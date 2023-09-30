#pragma once

#include <string>

namespace rsl {

// Get a thousands-separated string e.g. "10,000" from a number |x|.
std::string ToEnUsString(auto&& x) {
  auto str = std::to_string(x);
  const std::size_t decimal_pos = str.find('.');
  // If there's no decimal point, we want to start inserting commas three
  // characters from the right. If there is a decimal point, we want to start
  // inserting commas three characters to the left of the decimal point.
  std::ptrdiff_t insert_pos =
      (decimal_pos == std::string::npos)
          ? std::ssize(str) - 3
          : static_cast<std::ptrdiff_t>(decimal_pos) - 3;

  // Insert commas until we've processed the entire string
  while (insert_pos > 0) {
    str.insert(insert_pos, ",");
    insert_pos -= 3;
  }

  return str;
}

} // namespace rsl
