#pragma once

#include "SZS.hpp"
#include <algorithm>
#include <string.h>

namespace rlibrii::szs {

static inline void compressionSearch(const u8* src_pos, const u8* src,
                                     int max_len, u32 search_range,
                                     const u8* src_end, const u8** out_found,
                                     u32* out_found_len) {
  u32& found_len = *out_found_len;
  const u8*& found = *out_found;

  found_len = 0;
  found = nullptr;

  const u8* search;
  const u8* cmp_end;
  u8 c1;
  const u8* cmp1;
  const u8* cmp2;
  int len_;

  if (src_pos + 2 < src_end) {
    search = src_pos - search_range;
    if (search < src)
      search = src;

    cmp_end = src_pos + max_len;
    if (cmp_end > src_end)
      cmp_end = src_end;

    c1 = *src_pos;
    while (search < src_pos) {
      search = (u8*)memchr(search, c1, src_pos - search);
      if (search == nullptr)
        break;

      cmp1 = search + 1;
      cmp2 = src_pos + 1;

      while (cmp2 < cmp_end && *cmp1 == *cmp2) {
        cmp1++;
        cmp2++;
      }

      len_ = cmp2 - src_pos;

      if (found_len < len_) {
        found_len = len_;
        found = search;
        if (found_len == max_len)
          break;
      }

      search++;
    }
  }
}

static inline void CompressYaz(const u8* src, u32 src_len, u8 level, u8* dst,
                               u32 dst_len, u32* out_len) {
  writeU32(dst, 0x0, YAZ0_MAGIC);
  writeU32(dst, 0x4, src_len);
  writeU32(dst, 0x8, 0x0);
  writeU32(dst, 0xc, 0x0);

  // TOOD: Check dest_len
  u32 search_range;

  if (level == 0)
    search_range = 0;

  else if (level < 9)
    search_range = 0x10e0 * level / 9 - 0x0e0;

  else
    search_range = 0x1000;

  const u8* src_pos = src;
  const u8* src_end = src + src_len;

  u8* dst_pos = dst + 16;
  u8* code_byte = dst_pos;

  int max_len = 18 + 255;

  int i;

  u32 found_len, delta;
  const u8* found;
  u32 next_found_len;
  const u8* next_found;

  if (search_range == 0) {
    while (src_end - src_pos >= 8) {
      *dst_pos++ = 0xFF;
      memcpy(dst_pos, src_pos, 8);
      dst_pos += 8;
      src_pos += 8;
    }

    delta = src_end - src_pos;
    if (delta > 0) {
      *dst_pos = ((1 << delta) - 1) << (8 - delta);
      memcpy(dst_pos, src_pos, delta);
      dst_pos += delta;
      src_pos += delta;
    }
  } else if (src_pos < src_end) {
    compressionSearch(src_pos, src, max_len, search_range, src_end, &found,
                      &found_len);

    next_found = nullptr;
    next_found_len = 0;

    while (src_pos < src_end) {
      code_byte = dst_pos;
      *dst_pos++ = 0;

      for (i = 8 - 1; i >= 0; i--) {
        if (src_pos >= src_end)
          break;

        if (src_pos + 1 < src_end)
          compressionSearch(src_pos + 1, src, max_len, search_range, src_end,
                            &next_found, &next_found_len);

        if (found_len > 2 && next_found_len <= found_len) {
          delta = src_pos - found - 1;

          if (found_len < 18) {
            *dst_pos++ = delta >> 8 | (found_len - 2) << 4;
            *dst_pos++ = delta & 0xFF;
          } else {
            *dst_pos++ = delta >> 8;
            *dst_pos++ = delta & 0xFF;
            *dst_pos++ = (found_len - 18) & 0xFF;
          }

          src_pos += found_len;

          compressionSearch(src_pos, src, max_len, search_range, src_end,
                            &found, &found_len);
        } else {
          *code_byte |= 1 << i;
          *dst_pos++ = *src_pos++;

          found = next_found;
          found_len = next_found_len;
        }
      }
    }
  }

  *out_len = dst_pos - dst;
}

} // namespace rlibrii::szs
