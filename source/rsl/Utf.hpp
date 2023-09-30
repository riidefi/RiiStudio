#pragma once

#include "Try.hpp"
#include <rsl/Expected.hpp>
#include <stdint.h>
#include <string>

extern "C" int32_t rii_utf16_to_utf8(const uint16_t* utf16, uint32_t len,
                                     char* output, uint32_t output_len);

namespace rsl {

static inline std::expected<void, std::string>
ConvertUTF16ToUTF8(std::span<const uint16_t> utf16, std::span<char> output) {
  if (utf16.size() < output.size()) {
    return std::unexpected("ConvertUTF16ToUTF8: Sizes do not match");
  }
  auto stat = rii_utf16_to_utf8(utf16.data(), utf16.size(), output.data(),
                                output.size());
  if (stat != 0) {
    return std::unexpected("ConvertUTF16ToUTF8: Conversion failure");
  }

  return {};
}

static inline std::expected<std::string, std::string>
StdStringFromUTF16(std::span<const uint16_t> utf16) {
  std::string result;
  result.resize(utf16.size());
  TRY(ConvertUTF16ToUTF8(utf16, result));
  return result;
}

} // namespace rsl
