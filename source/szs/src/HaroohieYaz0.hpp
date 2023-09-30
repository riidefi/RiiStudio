// @file HaroohieYaz0.hpp
//
// @author Gericom, (riidefi)
//
// @brief C++ port of HaroohiePals.IO.Compression.Yaz0
// https://github.com/HaroohiePals/MarioKartToolbox/blob/main/HaroohiePals.IO/Compression/Yaz0.cs
//
#pragma once

#include "HaroohieCompressionWindow.hpp"

#include <span>
#include <stdint.h>
#include <vector>

namespace HaroohiePals::Yaz0 {

static inline std::vector<uint8_t> Compress(std::span<const uint8_t> src) {
  CompressionWindow context(src, 3, 273, 4096);
  std::vector<uint8_t> dst;
  dst.reserve((src.size() + (src.size() >> 3) + 0x18) & ~0x7);

  dst.push_back('Y');
  dst.push_back('a');
  dst.push_back('z');
  dst.push_back('0');
  dst.push_back((src.size() >> 24) & 0xFF);
  dst.push_back((src.size() >> 16) & 0xFF);
  dst.push_back((src.size() >> 8) & 0xFF);
  dst.push_back(src.size() & 0xFF);

  for (int i = 0; i < 8; ++i) {
    dst.push_back(0);
  }

  while (context.Position < src.size()) {
    size_t blockStart = dst.size();
    dst.push_back(0);

    uint8_t header = 0;

    for (int i = 0; i < 8; i++) {
      header <<= 1;
      auto [pos, len] = context.FindRun();

      if (len > 0) {
        uint32_t back = context.Position - pos;

        if (len >= 18) {
          dst.push_back(((back - 1) >> 8) & 0xF);
          dst.push_back((back - 1) & 0xFF);
          dst.push_back((len - 0x12) & 0xFF);
        } else {
          dst.push_back((((back - 1) >> 8) & 0xF) |
                        ((((uint32_t)len - 2) & 0xF) << 4));
          dst.push_back((back - 1) & 0xFF);
        }

        context.Slide(len);
      } else {
        header |= 1;
        dst.push_back(src[context.Position]);
        context.Slide(1);
      }

      if (context.Position >= src.size()) {
        header <<= 7 - i;
        break;
      }
    }

    dst[blockStart] = header;
  }

  while ((dst.size() % 4) != 0)
    dst.push_back(0);

  return dst;
}

} // namespace HaroohiePals::Yaz0
