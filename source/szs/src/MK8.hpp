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
    cSearchWindowSize = 0x1000,
    cMatchLenMax = 0x111,

    cPosTempBufferNum = 0x8000,
    cSearchPosBufferNum = cSearchWindowSize,

    // In-game: 1x (~8KB)
    // Official tooling: 10x presumably
    // Better results attained with 100x (~1MB)
    cDataBufferSize = 0x2000 * 100,
    cPosTempBufferSize = cPosTempBufferNum * sizeof(s32),
    cSearchPosBufferSize = cSearchPosBufferNum * sizeof(s32),

    cWorkSize = cDataBufferSize + cPosTempBufferSize + cSearchPosBufferSize
  };

  struct Work {
    u8 data_buffer[cDataBufferSize];
    s32 pos_temp_buffer[cPosTempBufferNum];
    s32 search_pos_buffer[cSearchPosBufferNum];
  };
  static_assert(sizeof(Work) == cWorkSize);

  struct Match {
    s32 len;
    s32 pos;
  };

  class PosTempBufferIndex {
  public:
    PosTempBufferIndex() : mValue(0) {}

    void pushBack(u32 value) {
      mValue = ((mValue << 5) ^ value) % cPosTempBufferNum;
    }

    u32 value() const { return mValue; }

  private:
    u32 mValue;
  };

  static inline bool search(Match& match, s32 search_pos, const u32 data_pos,
                            const s32 data_buffer_size, const Work& work);
};

u32 CompressorFast::getRequiredMemorySize() { return cWorkSize; }

bool CompressorFast::search(Match& match, s32 search_pos, const u32 data_pos,
                            const s32 data_buffer_size, const Work& work) {
  const u8* cmp2 = work.data_buffer + data_pos;

  s32 search_pos_min =
      data_pos > cSearchWindowSize ? s32(data_pos - cSearchWindowSize) : -1;

  s32 match_len = 2;
  match.len = match_len;

  if (data_pos - search_pos <= cSearchWindowSize) {
    for (u32 i = 0; i < cSearchWindowSize; i++) {
      const u8* cmp1 = work.data_buffer + search_pos;
      if (cmp1[0] == cmp2[0] && cmp1[1] == cmp2[1] &&
          cmp1[match_len] == cmp2[match_len]) {
        s32 len_local;

        for (len_local = 2; len_local < cMatchLenMax; len_local++)
          if (cmp1[len_local] != cmp2[len_local])
            break;

        if (len_local > match_len) {
          match.len = len_local;
          match.pos = cmp2 - cmp1;

          match_len = data_buffer_size;
          if (match.len <= match_len)
            match_len = match.len;
          else
            match.len = match_len;

          if (len_local >= cMatchLenMax)
            break;
        }
      }

      search_pos = work.search_pos_buffer[search_pos % cSearchPosBufferNum];
      if (search_pos <= search_pos_min)
        break;
    }

    if (match_len >= 3)
      return true;
  }

  return false;
}

u32 CompressorFast::encode(u8* p_dst, const u8* p_src, u32 src_size,
                           u8* p_work) {
  Work& work = *reinterpret_cast<Work*>(p_work);
  u8 temp_buffer[24];
  u32 temp_size = 0;

  s32 search_pos = -1;
  bool found_next_match = false;
  s32 bit = 8;

  u32 data_pos;
  s32 data_buffer_size;

  memset(work.pos_temp_buffer, u8(-1), cPosTempBufferSize);
  memset(work.search_pos_buffer, u8(-1), cSearchPosBufferSize);

  data_pos = 0;

  u32 out_size = 0x10; // Header size
  u32 flag = 0;

  memcpy(p_dst, "Yaz0", 4);
  p_dst[4] = (src_size >> 24) & 0xff;
  p_dst[5] = (src_size >> 16) & 0xff;
  p_dst[6] = (src_size >> 8) & 0xff;
  p_dst[7] = (src_size >> 0) & 0xff;
  memset(p_dst + 8, 0, 0x10); // They probably meant to put 8 in the size here?

  PosTempBufferIndex pos_temp_buffer_idx;

  data_buffer_size = seadMathMin<u32>(cDataBufferSize, src_size);
  memcpy(work.data_buffer, p_src, data_buffer_size);

  pos_temp_buffer_idx.pushBack(work.data_buffer[0]);
  pos_temp_buffer_idx.pushBack(work.data_buffer[1]);

  Match match, next_match;
  match.len = 2;

  s32 current_read_end_pos = data_buffer_size;
  s32 next_read_end_pos;

  while (data_buffer_size > 0) {
    while (true) {
      if (!found_next_match) {
        pos_temp_buffer_idx.pushBack(work.data_buffer[data_pos + 2]);

        work.search_pos_buffer[data_pos % cSearchPosBufferNum] =
            work.pos_temp_buffer[pos_temp_buffer_idx.value()];
        work.pos_temp_buffer[pos_temp_buffer_idx.value()] = data_pos;

        search_pos = work.search_pos_buffer[data_pos % cSearchPosBufferNum];
      } else {
        found_next_match = false;
      }

      if (search_pos != -1) {
        search(match, search_pos, data_pos, data_buffer_size, work);

        if (2 < match.len && match.len < cMatchLenMax) {
          data_pos++;
          data_buffer_size--;

          pos_temp_buffer_idx.pushBack(work.data_buffer[data_pos + 2]);

          work.search_pos_buffer[data_pos % cSearchPosBufferNum] =
              work.pos_temp_buffer[pos_temp_buffer_idx.value()];
          work.pos_temp_buffer[pos_temp_buffer_idx.value()] = data_pos;

          search_pos = work.search_pos_buffer[data_pos % cSearchPosBufferNum];

          search(next_match, search_pos, data_pos, data_buffer_size, work);

          if (match.len < next_match.len)
            match.len = 2;

          found_next_match = true;
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

        data_buffer_size -= match.len - s32(found_next_match);
        match.len -= s32(found_next_match) + 1;

        do {
          data_pos++;

          pos_temp_buffer_idx.pushBack(work.data_buffer[data_pos + 2]);

          work.search_pos_buffer[data_pos % cSearchPosBufferNum] =
              work.pos_temp_buffer[pos_temp_buffer_idx.value()];
          work.pos_temp_buffer[pos_temp_buffer_idx.value()] = data_pos;

          search_pos = work.search_pos_buffer[data_pos % cSearchPosBufferNum];
        } while (--match.len != 0);

        data_pos++;
        found_next_match = false;
        match.len = 0;
      } else {
        flag = (flag & 0x7f) << 1 | 1;

        temp_buffer[temp_size++] =
            work.data_buffer[data_pos - s32(found_next_match)];

        if (!found_next_match) {
          data_pos++;
          data_buffer_size--;
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

      if (data_buffer_size < cMatchLenMax + 2)
        break;
    }

    s32 copy_pos =
        data_pos - cSearchWindowSize; // For search window for compression of
                                      // next src read portion
    s32 copy_size = cDataBufferSize - copy_pos;

    next_read_end_pos = current_read_end_pos;

    if (data_pos >= cSearchWindowSize + 14 * cMatchLenMax) {
      memcpy(work.data_buffer, work.data_buffer + copy_pos, copy_size);

      s32 next_read_size = cDataBufferSize - copy_size;
      next_read_end_pos = current_read_end_pos + next_read_size;
      if (src_size < u32(next_read_end_pos)) {
        next_read_size = src_size - current_read_end_pos;
        next_read_end_pos = src_size;
      }
      memcpy(work.data_buffer + copy_size, p_src + current_read_end_pos,
             next_read_size);
      data_buffer_size += next_read_size;
      data_pos -= copy_pos;

      for (u32 i = 0; i < cPosTempBufferNum; i++)
        work.pos_temp_buffer[i] = work.pos_temp_buffer[i] >= copy_pos
                                      ? work.pos_temp_buffer[i] - copy_pos
                                      : -1;

      for (u32 i = 0; i < cSearchPosBufferNum; i++)
        work.search_pos_buffer[i] = work.search_pos_buffer[i] >= copy_pos
                                        ? work.search_pos_buffer[i] - copy_pos
                                        : -1;
    }

    current_read_end_pos = next_read_end_pos;
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
