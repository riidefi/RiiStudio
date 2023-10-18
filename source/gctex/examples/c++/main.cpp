// See
// https://github.com/riidefi/RiiStudio/blob/master/source/gctex/include/gctex.h
//
// Main APIs:
//
// std::vector<uint8_t> encode(uint32_t format,
//                       std::span<const uint8_t> src,
//                       uint32_t width, uint32_t height);
//
// std::vector<uint8_t> decode(std::span<const uint8_t> src,
//                        uint32_t width, uint32_t height,
//                        uint32_t texformat,
//                        std::span<const uint8_t> tlut,
//                        uint32_t tlutformat);
#include "gctex.h"

#include <array>
#include <iostream>
#include <vector>

#define TEXFMT_CMPR 0xE

int main() {
  std::string version = gctex::get_version();
  std::cout << "gctex version: " << version << std::endl;

  uint32_t width = 2;
  uint32_t height = 2;
  std::array<uint8_t, 4 * 4> square{
      0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
  };

  // Encode as CMPR
  std::vector<uint8_t> encoded =
      gctex::encode(TEXFMT_CMPR, square, width, height);

  // Decode back from CMPR
  std::vector<uint8_t> decoded =
      gctex::decode(encoded, width, height, TEXFMT_CMPR, {}, 0);

  return 0;
}
