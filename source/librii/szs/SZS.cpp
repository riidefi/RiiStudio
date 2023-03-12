#include "SZS.hpp"
#include <oishii/writer/binary_writer.hxx>

namespace librii::szs {

Result<u32> getExpandedSize(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return std::unexpected("File too small to be a YAZ0 file");

  EXPECT(src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0');
  return (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
}

Result<void> decode(std::span<u8> dst, std::span<const u8> src) {
  EXPECT(dst.size() >= TRY(getExpandedSize(src)));

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

std::vector<u8> encodeFast(std::span<const u8> src) {
  std::vector<u8> result(16 + roundUp(src.size(), 8) / 8 * 9 - 1);

  result[0] = 'Y';
  result[1] = 'a';
  result[2] = 'z';
  result[3] = '0';

  result[4] = (src.size() & 0xff00'0000) >> 24;
  result[5] = (src.size() & 0x00ff'0000) >> 16;
  result[6] = (src.size() & 0x0000'ff00) >> 8;
  result[7] = (src.size() & 0x0000'00ff) >> 0;

  std::fill(result.begin() + 8, result.begin() + 16, 0);

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

} // namespace librii::szs
