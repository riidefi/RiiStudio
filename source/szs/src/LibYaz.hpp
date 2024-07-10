#pragma once

#include "SZS.hpp"
#include <algorithm>
#include <string.h>

namespace rlibrii::szs {

struct SearchResult {
  u32 found;
  u32 found_len;
};

static inline SearchResult compressionSearch(u32 src_pos,
                                             std::span<const u8> src_,
                                             int max_len, u32 search_range) {
  const u8* src = src_.data();
  const u8* src_end = src + src_.size();
  SearchResult result;
  result.found_len = 0;
  result.found = 0;

  const u8* search;
  const u8* cmp_end;
  u8 c1;
  const u8* cmp1;
  const u8* cmp2;
  int len_;

  if (src_pos + 2 < src_.size()) {
    const u8* src_ptr = src + src_pos;
    search = src_ptr - search_range;
    if (search < src)
      search = src;

    cmp_end = src_ptr + max_len;
    if (cmp_end > src_end)
      cmp_end = src_end;

    c1 = *src_ptr;
    while (search < src_ptr) {
      search = (u8*)memchr(search, c1, src_ptr - search);
      if (search == nullptr)
        break;

      cmp1 = search + 1;
      cmp2 = src_ptr + 1;

      while (cmp2 < cmp_end && *cmp1 == *cmp2) {
        cmp1++;
        cmp2++;
      }

      len_ = static_cast<int>(cmp2 - src_ptr);

      if (result.found_len < static_cast<u32>(len_)) {
        result.found_len = len_;
        result.found = search - src;
        if (result.found_len == max_len)
          break;
      }

      search++;
    }
  }

  return result;
}

static inline u32 CompressYaz_(const u8* src, u32 src_len, u8 level, u8* dst,
                               u32 dst_len) {
  (void)dst_len;
  writeU32(dst, 0x0, YAZ0_MAGIC);
  writeU32(dst, 0x4, src_len);
  writeU32(dst, 0x8, 0x0);
  writeU32(dst, 0xc, 0x0);

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
    auto res =
        compressionSearch(src_pos - src, std::span<const u8>(src, src_end),
                          max_len, search_range);
    found = src + res.found;
    found_len = res.found_len;

    next_found = nullptr;
    next_found_len = 0;

    while (src_pos < src_end) {
      code_byte = dst_pos;
      *dst_pos++ = 0;

      for (i = 8 - 1; i >= 0; i--) {
        if (src_pos >= src_end)
          break;

        if (src_pos + 1 < src_end) {
          auto res_ = compressionSearch(src_pos - src + 1,
                                        std::span<const u8>(src, src_end),
                                        max_len, search_range);
          next_found = src + res_.found;
          next_found_len = res_.found_len;
        }

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

          auto res_ = compressionSearch(src_pos - src,
                                        std::span<const u8>(src, src_end),
                                        max_len, search_range);
          found = src + res_.found;
          found_len = res_.found_len;
        } else {
          *code_byte |= 1 << i;
          *dst_pos++ = *src_pos++;

          found = next_found;
          found_len = next_found_len;
        }
      }
    }
  }

  u32 out_len = dst_pos - dst;
  return out_len;
}

static inline void CompressYaz(const u8* src, u32 src_len, u8 level, u8* dst,
                               u32 dst_len, u32* out_len) {
  *out_len = CompressYaz_(src, src_len, level, dst, dst_len);
}

} // namespace rlibrii::szs
