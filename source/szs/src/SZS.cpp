#include "SZS.hpp"

#include "CTLib.hpp"
#include "HaroohieYaz0.hpp"

#include <algorithm>
#include <string.h>

namespace rlibrii::szs {

u32 CompressMK8(const u8* src, u32 src_len, u8* dst, u32 dst_len);

Result<std::vector<u8>> encodeAlgo(std::span<const u8> buf, Algo algo) {
  if (algo == Algo::WorstCaseEncoding) {
    return encodeFast(buf);
  }
  if (algo == Algo::Nintendo) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    int sz = encodeBoyerMooreHorspool(buf.data(), tmp.data(), buf.size());
    if (sz < 0 || sz > tmp.size()) {
      return tl::unexpected("encodeBoyerMooreHorspool failed");
    }
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::MkwSp) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    u32 sz = encodeSP(buf.data(), tmp.data(), buf.size(), tmp.size());
    if (sz > tmp.size()) {
      return tl::unexpected("encodeSP failed");
    }
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::CTGP) {
    return encodeCTGP(buf);
  }
  if (algo == Algo::Haroohie) {
    return HaroohiePals::Yaz0::Compress(buf);
  }
  if (algo == Algo::CTLib) {
    return CTLib::compressBase(buf);
  }
  if (algo == Algo::LibYaz0) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    u32 sz = 0;
    CompressYaz(buf.data(), buf.size(), 9, tmp.data(), tmp.size(), &sz);
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::MK8) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    u32 sz = CompressMK8(buf.data(), buf.size(), tmp.data(), tmp.size());
    tmp.resize(sz);
    return tmp;
  }

  return tl::unexpected("Invalid algorithm: id=" + std::to_string((int)algo));
}

bool isDataYaz0Compressed(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return false;

  return src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0';
}

Result<u32> getExpandedSize(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return tl::unexpected("File too small to be a YAZ0 file");

  if (!(src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0')) {
    return tl::unexpected("Data is not a YAZ0 file");
  }
  return (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
}

Result<void> decode(std::span<u8> dst, std::span<const u8> src) {
  auto exp = getExpandedSize(src);
  if (!exp) {
    return tl::unexpected("Source is not a SZS compressed file!");
  }
  if (dst.size() < *exp) {
    return tl::unexpected("Result buffer is too small!");
  }

  int in_position = 0x10;
  int out_position = 0;

  const auto take8 = [&]() {
    const u8 result = src[in_position];
    ++in_position;
    return result;
  };
  const auto take16 = [&]() {
    const u32 high = src[in_position];
    ++in_position;
    const u32 low = src[in_position];
    ++in_position;
    return (high << 8) | low;
  };
  const auto give8 = [&](u8 val) {
    dst[out_position] = val;
    ++out_position;
  };

  const auto read_group = [&](bool raw) {
    if (raw) {
      give8(take8());
      return;
    }

    const u16 group = take16();
    const int reverse = (group & 0xfff) + 1;
    const int g_size = group >> 12;

    int size = g_size ? g_size + 2 : take8() + 18;

    // Invalid data could buffer overflow

    for (int i = 0; i < size; ++i) {
      give8(dst[out_position - reverse]);
    }
  };

  const auto read_chunk = [&]() {
    const u8 header = take8();

    for (int i = 0; i < 8; ++i) {
      if (in_position >= src.size() || out_position >= dst.size())
        return;
      read_group(header & (1 << (7 - i)));
    }
  };

  while (in_position < src.size() && out_position < dst.size())
    read_chunk();

  // if (out_position < dst.size())
  //   return llvm::createStringError(
  //       std::errc::executable_format_error,
  //       "Truncated source file: the file could not be decompressed fully");

  // if (in_position != src.size())
  //   return llvm::createStringError(
  //       std::errc::no_buffer_space,
  //       "Invalid YAZ0 header: file is larger than reported");

  return {};
}

u32 getWorstEncodingSize(u32 src) { return 16 + roundUp(src, 8) / 8 * 9 - 1; }
u32 getWorstEncodingSize(std::span<const u8> src) {
  return getWorstEncodingSize(static_cast<u32>(src.size()));
}
std::vector<u8> encodeFast(std::span<const u8> src) {
  std::vector<u8> result(getWorstEncodingSize(src));

  result[0] = 'Y';
  result[1] = 'a';
  result[2] = 'z';
  result[3] = '0';

  result[4] = (src.size() & 0xff00'0000) >> 24;
  result[5] = (src.size() & 0x00ff'0000) >> 16;
  result[6] = (src.size() & 0x0000'ff00) >> 8;
  result[7] = (src.size() & 0x0000'00ff) >> 0;

  std::fill(result.begin() + 8, result.begin() + 16, 0);

// Old implementation: Marginally slower but easier to read/reason about
#if 0
  auto* dst = result.data() + 16;

  unsigned counter = 0;
  for (auto* it = src.data(); it < src.data() + src.size(); --counter) {
    if (!counter) {
      *dst++ = 0xff;
      counter = 8;
    }
    *dst++ = *it++;
  }
  while (counter--)
    *dst++ = 0;
#else
  // New implementation courtesy abood

  auto* dst_pos = result.data() + 16;
  auto* src_pos = src.data();
  auto* src_end = src.data() + src.size();

  while (src_end - src_pos >= 8) {
    *dst_pos++ = 0xFF;
    memcpy(dst_pos, src_pos, 8);
    dst_pos += 8;
    src_pos += 8;
  }

  u32 delta = src_end - src_pos;
  if (delta > 0) {
    *dst_pos = ((1 << delta) - 1) << (8 - delta);
    memcpy(dst_pos + 1, src_pos, delta); // +1 to skip the marker byte
    dst_pos += delta + 1;                // +1 to account for the marker byte
    src_pos += delta;
  }
#endif

  return result;
}

static u16 sSkipTable[256];

static void findMatch(const u8* src, int srcPos, int maxSize, int* matchOffset,
                      int* matchSize);
static int searchWindow(const u8* needle, int needleSize, const u8* haystack,
                        int haystackSize);
static void computeSkipTable(const u8* needle, int needleSize);

int encodeBoyerMooreHorspool(const u8* src, u8* dst, int srcSize) {
  int srcPos;
  int groupHeaderPos;
  int dstPos;
  u8 groupHeaderBitRaw;

  dst[0] = 'Y';
  dst[1] = 'a';
  dst[2] = 'z';
  dst[3] = '0';
  dst[4] = 0;
  dst[5] = srcSize >> 16;
  dst[6] = srcSize >> 8;
  dst[7] = srcSize;
  dst[16] = 0;

  srcPos = 0;
  groupHeaderBitRaw = 0x80;
  groupHeaderPos = 16;
  dstPos = 17;
  while (srcPos < srcSize) {
    int matchOffset;
    int firstMatchLen;
    findMatch(src, srcPos, srcSize, &matchOffset, &firstMatchLen);
    if (firstMatchLen > 2) {
      int secondMatchOffset;
      int secondMatchLen;
      findMatch(src, srcPos + 1, srcSize, &secondMatchOffset, &secondMatchLen);
      if (firstMatchLen + 1 < secondMatchLen) {
        // Put a single byte
        dst[groupHeaderPos] |= groupHeaderBitRaw;
        groupHeaderBitRaw = groupHeaderBitRaw >> 1;
        dst[dstPos++] = src[srcPos++];
        if (!groupHeaderBitRaw) {
          groupHeaderBitRaw = 0x80;
          groupHeaderPos = dstPos;
          dst[dstPos++] = 0;
        }
        // Use the second match
        firstMatchLen = secondMatchLen;
        matchOffset = secondMatchOffset;
      }
      matchOffset = srcPos - matchOffset - 1;
      if (firstMatchLen < 18) {
        matchOffset |= ((firstMatchLen - 2) << 12);
        dst[dstPos] = matchOffset >> 8;
        dst[dstPos + 1] = matchOffset;
        dstPos += 2;
      } else {
        dst[dstPos] = matchOffset >> 8;
        dst[dstPos + 1] = matchOffset;
        dst[dstPos + 2] = firstMatchLen - 18;
        dstPos += 3;
      }
      srcPos += firstMatchLen;
    } else {
      // Put a single byte
      dst[groupHeaderPos] |= groupHeaderBitRaw;
      dst[dstPos++] = src[srcPos++];
    }

    // Write next group header
    groupHeaderBitRaw >>= 1;
    if (!groupHeaderBitRaw) {
      groupHeaderBitRaw = 0x80;
      groupHeaderPos = dstPos;
      dst[dstPos++] = 0;
    }
  }

  return dstPos;
}

void findMatch(const u8* src, int srcPos, int maxSize, int* matchOffset,
               int* matchSize) {
  // SZS backreference types:
  // (2 bytes) N >= 2:  NR RR    -> maxMatchSize=16+2,    windowOffset=4096+1
  // (3 bytes) N >= 18: 0R RR NN -> maxMatchSize=0xFF+18, windowOffset=4096+1
  int window = srcPos > 4096 ? srcPos - 4096 : 0;
  int windowSize = 3;
  int maxMatchSize = (maxSize - srcPos) <= 273 ? maxSize - srcPos : 273;
  if (maxMatchSize < 3) {
    *matchSize = 0;
    *matchOffset = 0;
    return;
  }

  int windowOffset;
  int foundMatchOffset;
  while (window < srcPos &&
         (windowOffset = searchWindow(&src[srcPos], windowSize, &src[window],
                                      srcPos + windowSize - window)) <
             srcPos - window) {
    for (; windowSize < maxMatchSize; ++windowSize) {
      if (src[window + windowOffset + windowSize] != src[srcPos + windowSize])
        break;
    }
    if (windowSize == maxMatchSize) {
      *matchOffset = window + windowOffset;
      *matchSize = maxMatchSize;
      return;
    }
    foundMatchOffset = window + windowOffset;
    ++windowSize;
    window += windowOffset + 1;
  }
  *matchOffset = foundMatchOffset;
  *matchSize = windowSize > 3 ? windowSize - 1 : 0;
}

static int searchWindow(const u8* needle, int needleSize, const u8* haystack,
                        int haystackSize) {
  int itHaystack; // r8
  int itNeedle;   // r9

  if (needleSize > haystackSize)
    return haystackSize;
  computeSkipTable(needle, needleSize);

  // Scan forwards for the last character in the needle
  for (itHaystack = needleSize - 1;;) {
    while (1) {
      if (needle[needleSize - 1] == haystack[itHaystack])
        break;
      itHaystack += sSkipTable[haystack[itHaystack]];
    }
    --itHaystack;
    itNeedle = needleSize - 2;
    break;
  Difference:
    // The entire needle was not found, continue search
    int skip = sSkipTable[haystack[itHaystack]];
    if (needleSize - itNeedle > skip)
      skip = needleSize - itNeedle;
    itHaystack += skip;
  }

  // Scan backwards for the first difference
  int remainingBytes = itNeedle;
  for (int j = 0; j <= remainingBytes; ++j) {
    if (haystack[itHaystack] != needle[itNeedle]) {
      goto Difference;
    }
    --itHaystack;
    --itNeedle;
  }
  return itHaystack + 1;
}

static void computeSkipTable(const u8* needle, int needleSize) {
  for (int i = 0; i < 256; ++i) {
    sSkipTable[i] = needleSize;
  }
  for (int i = 0; i < needleSize; ++i) {
    sSkipTable[needle[i]] = needleSize - i - 1;
  }
}

enum {
  YAZ0_MAGIC = 0x59617a30,
  YAZ1_MAGIC = 0x59617a31,
};

static void writeU16(u8* data, u32 offset, u16 val) {
  u8* base = data + offset;
  base[0x0] = val >> 8;
  base[0x1] = val;
}

static void writeU32(u8* data, u32 offset, u32 val) {
  u8* base = data + offset;
  base[0x0] = val >> 24;
  base[0x1] = val >> 16;
  base[0x2] = val >> 8;
  base[0x3] = val;
}

u32 encodeSP(const u8* src, u8* dst, u32 srcSize, u32 dstSize) {
  if (dstSize < 0x10) {
    return 0;
  }
  writeU32(dst, 0x0, YAZ0_MAGIC);
  writeU32(dst, 0x4, srcSize);
  writeU32(dst, 0x8, 0x0);
  writeU32(dst, 0xc, 0x0);

  u32 srcOffset = 0x0, dstOffset = 0x10;
  u32 groupHeaderOffset;
  for (u32 i = 0; srcOffset < srcSize && dstOffset < dstSize; i = (i + 1) % 8) {
    if (i == 0) {
      groupHeaderOffset = dstOffset;
      dst[dstOffset++] = 0;
      if (dstOffset == dstSize) {
        return 0;
      }
    }
    u32 firstRefOffset = srcOffset < 0x1000 ? 0x0 : srcOffset - 0x1000;
    u32 bestRefSize = 0x1, bestRefOffset;
    for (u32 refOffset = firstRefOffset; refOffset < srcOffset; refOffset++) {
      u32 refSize;
      u32 maxRefSize = 0x111;
      if (srcSize - srcOffset < maxRefSize) {
        maxRefSize = srcSize - srcOffset;
      }
      if (dstSize - dstOffset < maxRefSize) {
        maxRefSize = dstSize - dstOffset;
      }
      if (bestRefSize < maxRefSize) {
        if (src[srcOffset + bestRefSize] != src[refOffset + bestRefSize]) {
          continue;
        }
        for (refSize = 0; refSize < maxRefSize; refSize++) {
          if (src[srcOffset + refSize] != src[refOffset + refSize]) {
            break;
          }
        }
        if (refSize > bestRefSize) {
          bestRefSize = refSize;
          bestRefOffset = refOffset;
          if (bestRefSize == 0x111) {
            break;
          }
        }
      } else {
        break;
      }
    }
    if (bestRefSize < 0x3) {
      dst[groupHeaderOffset] |= 1 << (7 - i);
      dst[dstOffset++] = src[srcOffset++];
    } else {
      if (bestRefSize < 0x12) {
        if (dstOffset + sizeof(u16) > dstSize) {
          return 0;
        }
        u16 val = (bestRefSize - 0x2) << 12 | (srcOffset - bestRefOffset - 0x1);
        writeU16(dst, dstOffset, val);
        dstOffset += sizeof(u16);
      } else {
        if (dstOffset + sizeof(u16) > dstSize) {
          return 0;
        }
        writeU16(dst, dstOffset, srcOffset - bestRefOffset - 0x1);
        dstOffset += sizeof(u16);
        if (dstOffset + sizeof(u8) > dstSize) {
          return 0;
        }
        dst[dstOffset++] = bestRefSize - 0x12;
      }
      srcOffset += bestRefSize;
    }
  }

  return srcOffset == srcSize ? dstOffset : 0;
}
static void compressionSearch(const u8* src_pos, const u8* src, int max_len,
                              u32 search_range, const u8* src_end,
                              const u8** out_found, u32* out_found_len) {
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

void CompressYaz(const u8* src, u32 src_len, u8 level, u8* dst, u32 dst_len,
                 u32* out_len) {
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

// Credit to Killz for spotting this and Abood for decompiling it.
template <typename T> inline T seadMathMin(T a, T b) {
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

u32 CompressMK8(const u8* src, u32 src_len, u8* dst, u32 dst_len) {
  util::CompressorFast compressor;
  std::vector<u8> work(compressor.getRequiredMemorySize());
  return compressor.encode(dst, src, src_len, work.data());
}

} // namespace rlibrii::szs
