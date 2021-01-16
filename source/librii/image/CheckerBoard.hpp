#pragma once

#include <core/common.h>

namespace librii::image {

inline void generateCheckerboard(u8* scratch, int width, int height,
                                 u32 fg = 0xff'00'dc'ff,
                                 u32 bg = 0x00'00'00'ff) {
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      const auto px_offset = i * width * 4 + j * 4;
      const bool is_fg = ((i / 8) & 1) != ((j / 8) & 1);
      const u32 color = is_fg ? fg : bg;
      scratch[px_offset] = (color & 0xff'00'00'00) >> 24;
      scratch[px_offset + 1] = (color & 0x00'ff'00'00) >> 16;
      scratch[px_offset + 2] = (color & 0x00'00'ff'00) >> 8;
      scratch[px_offset + 3] = (color & 0x00'00'00'ff);
    }
  }
}

} // namespace librii::image
