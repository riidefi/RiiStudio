#pragma once

#include "SZS.hpp"
#include <algorithm>
#include <assert.h>
#include <span>
#include <string.h>

namespace util {

// Credit to Killz for spotting this and Abood for decompiling it.
template <typename T> static inline T seadMathMin(T a, T b) {
  if (a < b)
    return a;
  else
    return b;
}

constexpr int SEARCH_WINDOW_SIZE = 0x1000;
constexpr int MATCH_LEN_MAX = 0x111;

constexpr int POS_TEMP_BUFFER_NUM = 0x8000;
constexpr int SEARCH_POS_BUFFER_NUM = SEARCH_WINDOW_SIZE;

// In-game: 1x (~8KB)
// Official tooling: 10x presumably
// Better results attained with 100x (~1MB)
constexpr int DATA_BUFFER_SIZE = 0x2000 * 100;

constexpr int POS_TEMP_BUFFER_SIZE = POS_TEMP_BUFFER_NUM * sizeof(s32);
constexpr int SEARCH_POS_BUFFER_SIZE = SEARCH_POS_BUFFER_NUM * sizeof(s32);
constexpr int WORK_SIZE =
    DATA_BUFFER_SIZE + POS_TEMP_BUFFER_SIZE + SEARCH_POS_BUFFER_SIZE;

struct Match {
  s32 len;
  s32 pos;
};

// The compressor uses a chained hash table to find duplicated strings,
//  using a hash function that operates on 3-byte sequences.  At any
//  given point during compression, let XYZ be the next 3 input bytes to
//  be examined (not necessarily all different, of course).
class TripletHasher {
public:
  TripletHasher() : mValue(0) {}

  void pushBack(u32 value) {
    mValue = ((mValue << 5) ^ value) % POS_TEMP_BUFFER_NUM;
  }

  u32 getHash() const { return mValue; }

private:
  u32 mValue;
};

struct TempBuffer {
  u8 temp_buffer[24];
  u32 temp_size = 0;

  void put(u8 x) {
    temp_buffer[temp_size] = x;
    temp_size += 1;
  }
};

struct Work {
  u8 data_buffer[DATA_BUFFER_SIZE];
  s32 pos_temp_buffer[POS_TEMP_BUFFER_NUM];
  s32 search_pos_buffer[SEARCH_POS_BUFFER_NUM];

  s32& search_pos(u32 x) {
    return search_pos_buffer[x % SEARCH_POS_BUFFER_NUM];
  }
  const s32& search_pos(u32 x) const {
    return search_pos_buffer[x % SEARCH_POS_BUFFER_NUM];
  }
  u32 appendToHashChain(u32 data_pos,
                        const TripletHasher& pos_temp_buffer_idx) {
    search_pos_buffer[data_pos % SEARCH_POS_BUFFER_NUM] =
        pos_temp_buffer[pos_temp_buffer_idx.getHash()];
    pos_temp_buffer[pos_temp_buffer_idx.getHash()] = data_pos;
    return search_pos_buffer[data_pos % SEARCH_POS_BUFFER_NUM];
  }
};
static_assert(sizeof(Work) == WORK_SIZE);

static inline u32 getRequiredMemorySize() { return WORK_SIZE; }

static inline bool search(Match& match, const s32 search_pos_immut,
                          const u32 data_pos, const s32 data_buffer_size,
                          const Work& work) {
  s32 search_pos = search_pos_immut;
  std::span<const u8> cmp2(work.data_buffer + data_pos,
                           DATA_BUFFER_SIZE - data_pos);

  assert(search_pos >= 0);

  // Maximum backtrace amount, either the maximum backtrace for a SZS token (12
  // bits => 0x1000) or how much actually precedes the buffer.
  s32 search_pos_min =
      data_pos > SEARCH_WINDOW_SIZE ? s32(data_pos - SEARCH_WINDOW_SIZE) : -1;

  s32 best_match_len = 2;
  match.len = best_match_len;

  // There are no deletions from the hash chains; the algorithm
  //  simply discards matches that are too old.
  if (data_pos - search_pos > SEARCH_WINDOW_SIZE) {
    return false;
  }

  // The compressor searches the hash chains starting with the most recent
  //  strings, to favor small distances and thus take advantage of the
  //  Huffman encoding.  The hash chains are singly linked.
  //
  //  To avoid a worst-case situation, very long hash
  //  chains are arbitrarily truncated at a certain length, determined by a
  //  run-time parameter [SEARCH_WINDOW_SIZE in this case].
  for (u32 i = 0; i < SEARCH_WINDOW_SIZE; i++) {
    std::span<const u8> cmp1(work.data_buffer + search_pos,
                             DATA_BUFFER_SIZE - search_pos);
    // Before calling some heavy vectorized comparison function on the entire
    // buffer, discard easy values:
    // - Hash conflicts (likely the first two bytes will be different)
    // - The last byte (The least likely byte to match given the longer the
    //   prefix the greater number of matches)
    //   (based on the best so far)
    bool isMatchCandidate = cmp1[0] == cmp2[0] && cmp1[1] == cmp2[1] &&
                            cmp1[static_cast<size_t>(best_match_len)] ==
                                cmp2[static_cast<size_t>(best_match_len)];
    if (isMatchCandidate) {
      s32 candidate_match_len = 2;

      // Search for longest common prefix
      while (candidate_match_len < MATCH_LEN_MAX) {
        if (cmp1[candidate_match_len] != cmp2[candidate_match_len]) {
          break;
        }
        candidate_match_len++;
      }

      if (candidate_match_len > best_match_len) {
        match.len = candidate_match_len;
        match.pos = static_cast<s32>(data_pos - search_pos);

        // Truncation based on the data buffer size
        best_match_len = data_buffer_size;
        if (static_cast<s64>(match.len) <= static_cast<s64>(best_match_len)) {
          best_match_len = match.len;
        } else {
          match.len = best_match_len;
        }

        // We can't do any better
        if (candidate_match_len >= MATCH_LEN_MAX) {
          break;
        }
      }
    }

    search_pos = work.search_pos(search_pos);
    if (search_pos <= search_pos_min) {
      break;
    }
  }

  if (best_match_len >= 3) {
    return true;
  }

  return false;
}

static inline u32 encode(u8* p_dst, const u8* p_src, u32 src_size, u8* p_work) {
  Work& work = *reinterpret_cast<Work*>(p_work);
  TempBuffer tmp;

  s32 search_pos = -1;
  bool found_next_match = false;
  s32 bit = 8;

  u32 data_pos;
  s32 data_buffer_size;

  memset(work.pos_temp_buffer, u8(-1), POS_TEMP_BUFFER_SIZE);
  memset(work.search_pos_buffer, u8(-1), SEARCH_POS_BUFFER_SIZE);

  data_pos = 0;

  u32 out_size = 0x10; // Header size
  u32 flag = 0;

  memcpy(p_dst, "Yaz0", 4);
  p_dst[4] = (src_size >> 24) & 0xff;
  p_dst[5] = (src_size >> 16) & 0xff;
  p_dst[6] = (src_size >> 8) & 0xff;
  p_dst[7] = (src_size >> 0) & 0xff;
  memset(p_dst + 8, 0, 8);

  TripletHasher xyz_hasher;

  data_buffer_size = seadMathMin<u32>(DATA_BUFFER_SIZE, src_size);
  memcpy(work.data_buffer, p_src, data_buffer_size);

  xyz_hasher.pushBack(work.data_buffer[0]);
  xyz_hasher.pushBack(work.data_buffer[1]);

  Match match_info, next_match;
  match_info.len = 2;
  next_match.len = 0;

  s32 current_read_end_pos = data_buffer_size;
  s32 next_read_end_pos;

  while (data_buffer_size > 0) {
    while (true) {
      if (!found_next_match) {
        xyz_hasher.pushBack(work.data_buffer[data_pos + 2]);
        search_pos = work.appendToHashChain(data_pos, xyz_hasher);
      } else {
        found_next_match = false;
      }

      if (search_pos != -1) {
        search(match_info, search_pos, data_pos, data_buffer_size, work);

        if (2 < match_info.len && match_info.len < MATCH_LEN_MAX) {
          data_pos += 1;
          data_buffer_size -= 1;

          xyz_hasher.pushBack(work.data_buffer[data_pos + 2]);
          search_pos = work.appendToHashChain(data_pos, xyz_hasher);

          if (search_pos != -1) {
            search(next_match, search_pos, data_pos, data_buffer_size, work);
            if (match_info.len < next_match.len)
              match_info.len = 2;
          }
          found_next_match = true;
        }
      }

      if (match_info.len > 2) {
        flag = (flag & 0x7f) << 1;

        u8 low = match_info.pos - 1;
        u8 high = (match_info.pos - 1) >> 8;

        if (match_info.len < 18) {
          tmp.put(u8((match_info.len - 2) << 4) | high);
          tmp.put(low);
        } else {
          tmp.put(high);
          tmp.put(low);
          tmp.put(u8(match_info.len) - 18);
        }

        data_buffer_size -= match_info.len - s32(found_next_match);
        match_info.len -= s32(found_next_match) + 1;

        for (;;) {
          data_pos++;

          xyz_hasher.pushBack(work.data_buffer[data_pos + 2]);

          search_pos = work.appendToHashChain(data_pos, xyz_hasher);
          match_info.len -= 1;
          if (match_info.len == 0) {
            break;
          }
        }

        data_pos++;
        found_next_match = false;
        match_info.len = 0;
      } else {
        flag = (flag & 0x7f) << 1 | 1;

        tmp.put(work.data_buffer[data_pos - s32(found_next_match)]);

        if (!found_next_match) {
          data_pos++;
          data_buffer_size--;
        }
      }

      bit -= 1;

      if (bit == 0) {
        p_dst[out_size++] = flag;

        memcpy(p_dst + out_size, tmp.temp_buffer, tmp.temp_size);
        out_size += tmp.temp_size;

        flag = 0;
        tmp.temp_size = 0;
        bit = 8;
      }

      if (data_buffer_size < MATCH_LEN_MAX + 2)
        break;
    }
    // For search window for compression of next src read portion
    s32 copy_pos = data_pos - SEARCH_WINDOW_SIZE;
    s32 copy_size = DATA_BUFFER_SIZE - copy_pos;

    next_read_end_pos = current_read_end_pos;

    if (data_pos >= SEARCH_WINDOW_SIZE + 14 * MATCH_LEN_MAX) {
      memcpy(work.data_buffer, work.data_buffer + copy_pos, copy_size);

      s32 next_read_size = DATA_BUFFER_SIZE - copy_size;
      next_read_end_pos = current_read_end_pos + next_read_size;
      if (src_size < u32(next_read_end_pos)) {
        next_read_size = src_size - current_read_end_pos;
        next_read_end_pos = src_size;
      }
      memcpy(work.data_buffer + copy_size, p_src + current_read_end_pos,
             next_read_size);
      data_buffer_size += next_read_size;
      data_pos -= copy_pos;

      for (u32 i = 0; i < POS_TEMP_BUFFER_NUM; i++) {
        work.pos_temp_buffer[i] = work.pos_temp_buffer[i] >= copy_pos
                                      ? work.pos_temp_buffer[i] - copy_pos
                                      : -1;
      }

      for (u32 i = 0; i < SEARCH_POS_BUFFER_NUM; i++) {
        work.search_pos_buffer[i] = work.search_pos_buffer[i] >= copy_pos
                                        ? work.search_pos_buffer[i] - copy_pos
                                        : -1;
      }
    }

    current_read_end_pos = next_read_end_pos;
  }

  flag = ((bit & 0x3f) == 8) ? 0 : (flag << (bit & 0x3f));

  p_dst[out_size++] = flag;
  memcpy(p_dst + out_size, tmp.temp_buffer, tmp.temp_size);
  out_size += tmp.temp_size;
  tmp.temp_size = 0;

  return out_size;
}

} // namespace util

// PUBLIC

namespace rlibrii::szs {

static inline u32 CompressMK8(std::span<const u8> src, std::span<u8> dst) {
  std::vector<u8> work(util::getRequiredMemorySize());
  return util::encode(dst.data(), src.data(), src.size(), work.data());
}

} // namespace rlibrii::szs
