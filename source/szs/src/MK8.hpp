#pragma once

#include "SZS.hpp"
#include <algorithm>
#include <string.h>

namespace rlibrii::szs {

// Credit to Killz for spotting this and Abood for decompiling it.
template <typename T> static inline T seadMathMin(T a, T b) {
  if (a < b)
    return a;
  else
    return b;
}

namespace util {

class CompressorFast {
public:
  static u32 getRequiredMemorySize();
  static u32 encode(u8* p_dst, const u8* p_src, u32 src_size, u8* p_work);

private:
  enum {
    cWorkNum1 = 0x8000,
    cWorkNum2 = 0x1000,

    cWorkSize0 = 0x2000,
    cWorkSize1 = cWorkNum1 * sizeof(s32),
    cWorkSize2 = cWorkNum2 * sizeof(s32)
  };

  struct Context {
    u8* p_buffer;
    u32 _4;
    s32* p_work_1;
    s32* p_work_2;
    s32 buffer_size;
  };

  struct Match {
    s32 len;
    s32 pos;
  };

  class PosIndex {
  public:
    PosIndex() : mValue(0) {}

    void pushBack(u32 value) {
      mValue <<= 5;
      mValue ^= value;
      mValue &= 0x7fff;
    }

    u32 value() const { return mValue; }

  private:
    u32 mValue;
  };

  static /* inline */ bool search(Match& match, s32 pos,
                                  const Context& context);
};

u32 CompressorFast::getRequiredMemorySize() {
  return cWorkSize0 + cWorkSize1 + cWorkSize2;
}

bool CompressorFast::search(Match& match, s32 pos, const Context& context) {
  const u8* cmp2 = context.p_buffer + context._4;

  s32 v0 = context._4 > 0x1000 ? s32(context._4 - 0x1000) : -1;

  s32 cmp_pos = 2;
  match.len = cmp_pos;

  if (context._4 - pos <= 0x1000) {
    for (u32 i = 0; i < 0x1000; i++) {
      const u8* cmp1 = context.p_buffer + pos;
      if (cmp1[0] == cmp2[0] && cmp1[1] == cmp2[1] &&
          cmp1[cmp_pos] == cmp2[cmp_pos]) {
        s32 len;

        for (len = 2; len < 0x111; len++)
          if (cmp1[len] != cmp2[len])
            break;

        if (len > cmp_pos) {
          match.len = len;
          match.pos = cmp2 - cmp1;

          cmp_pos = context.buffer_size;
          if (len <= cmp_pos)
            cmp_pos = match.len;
          else
            match.len = cmp_pos;

          if (len >= 0x111)
            break;
        }
      }

      pos = context.p_work_2[pos & 0xfff];
      if (pos <= v0)
        break;
    }

    if (cmp_pos >= 3)
      return true;
  }

  return false;
}

u32 CompressorFast::encode(u8* p_dst, const u8* p_src, u32 src_size,
                           u8* p_work) {
  u8 temp_buffer[24];
  u32 temp_size = 0;

  s32 pos = -1;
  s32 v1 = 0;
  s32 bit = 8;

  Context context;

  context.p_buffer = p_work;

  context.p_work_1 = (s32*)(p_work + cWorkSize0);
  memset(context.p_work_1, u8(-1), cWorkSize1);

  context.p_work_2 = (s32*)(p_work + (cWorkSize0 + cWorkSize1));
  memset(context.p_work_2, u8(-1), cWorkSize2);

  context._4 = 0;

  u32 out_size = 0x10; // Header size
  u32 flag = 0;

  memcpy(p_dst, "Yaz0", 4);
  p_dst[4] = (src_size >> 24) & 0xff;
  p_dst[5] = (src_size >> 16) & 0xff;
  p_dst[6] = (src_size >> 8) & 0xff;
  p_dst[7] = (src_size >> 0) & 0xff;
  memset(p_dst + 8, 0, 0x10); // They probably meant to put 8 in the size here?

  PosIndex v2;

  context.buffer_size = seadMathMin<u32>(cWorkSize0, src_size);
  memcpy(context.p_buffer, p_src, context.buffer_size);

  v2.pushBack(context.p_buffer[0]);
  v2.pushBack(context.p_buffer[1]);

  Match match, next_match;
  match.len = 2;

  s32 buffer_size_0 = context.buffer_size;
  s32 buffer_size_1;

  while (context.buffer_size > 0) {
    while (true) {
      if (v1 == 0) {
        v2.pushBack(context.p_buffer[context._4 + 2]);

        context.p_work_2[context._4 & 0xfff] = context.p_work_1[v2.value()];
        context.p_work_1[v2.value()] = context._4;

        pos = context.p_work_2[context._4 & 0xfff];
      } else {
        v1 = 0;
      }

      if (pos != -1) {
        search(match, pos, context);
        if (2 < match.len && match.len < 0x111) {
          context._4++;
          context.buffer_size--;

          v2.pushBack(context.p_buffer[context._4 + 2]);

          context.p_work_2[context._4 & 0xfff] = context.p_work_1[v2.value()];
          context.p_work_1[v2.value()] = context._4;

          pos = context.p_work_2[context._4 & 0xfff];
          search(next_match, pos, context);
          if (match.len < next_match.len)
            match.len = 2;

          v1 = 1;
        }
      }

      if (match.len > 2) {
        flag = (flag & 0x7f) << 1;

        u8 low = match.pos - 1;
        u8 high = (match.pos - 1) >> 8;

        if (match.len < 18) {
          temp_buffer[temp_size++] = u8((match.len - 2) << 4) | high;
          temp_buffer[temp_size++] = low;
        } else {
          temp_buffer[temp_size++] = high;
          temp_buffer[temp_size++] = low;
          temp_buffer[temp_size++] = u8(match.len) - 18;
        }

        context.buffer_size -= match.len - v1;
        match.len -= v1 + 1;

        do {
          context._4++;

          v2.pushBack(context.p_buffer[context._4 + 2]);

          context.p_work_2[context._4 & 0xfff] = context.p_work_1[v2.value()];
          context.p_work_1[v2.value()] = context._4;

          pos = context.p_work_2[context._4 & 0xfff];
        } while (--match.len != 0);

        context._4++;
        v1 = 0;
        match.len = 0;
      } else {
        flag = (flag & 0x7f) << 1 | 1;

        temp_buffer[temp_size++] = context.p_buffer[context._4 - v1];

        if (v1 == 0) {
          context._4++;
          context.buffer_size--;
        }
      }

      if (--bit == 0) {
        p_dst[out_size++] = flag;

        memcpy(p_dst + out_size, temp_buffer, temp_size);
        out_size += temp_size;

        flag = 0;
        temp_size = 0;
        bit = 8;
      }

      if (context.buffer_size < 0x111 + 2)
        break;
    }

    s32 v3 = context._4 - 0x1000;
    s32 v4 = cWorkSize0 - v3;

    buffer_size_1 = buffer_size_0;

    if (context._4 >= 0x1000 + 14 * 0x111) {
      memcpy(context.p_buffer, context.p_buffer + v3, v4);

      s32 v5 = cWorkSize0 - v4;
      buffer_size_1 = buffer_size_0 + v5;
      if (src_size < u32(buffer_size_1)) {
        v5 = src_size - buffer_size_0;
        buffer_size_1 = src_size;
      }
      memcpy(context.p_buffer + v4, p_src + buffer_size_0, v5);
      context.buffer_size += v5;
      context._4 -= v3;

      for (u32 i = 0; i < cWorkNum1; i++)
        context.p_work_1[i] =
            context.p_work_1[i] >= v3 ? context.p_work_1[i] - v3 : -1;

      for (u32 i = 0; i < cWorkNum2; i++)
        context.p_work_2[i] =
            context.p_work_2[i] >= v3 ? context.p_work_2[i] - v3 : -1;
    }
    buffer_size_0 = buffer_size_1;
  }

  p_dst[out_size++] = flag << (bit & 0x3f);

  memcpy(p_dst + out_size, temp_buffer, temp_size);
  out_size += temp_size;

  return out_size;
}

} // namespace util

static inline u32 CompressMK8(const u8* src, u32 src_len, u8* dst,
                              u32 dst_len) {
  util::CompressorFast compressor;
  std::vector<u8> work(compressor.getRequiredMemorySize());
  return compressor.encode(dst, src, src_len, work.data());
}

} // namespace rlibrii::szs
