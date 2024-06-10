#pragma once

#include "SZS.hpp"
#include <algorithm>
#include <string.h>

namespace rlibrii::szs {

struct EGGContext {
  u16 sSkipTable[256]{};
};

static void findMatch(EGGContext* ctx, const u8* src, int srcPos, int maxSize,
                      int* matchOffset, int* matchSize);
static int searchWindow(EGGContext* ctx, const u8* needle, int needleSize,
                        const u8* haystack, int haystackSize);
static void computeSkipTable(EGGContext* ctx, const u8* needle, int needleSize);

static inline int encodeBoyerMooreHorspool(const u8* src, u8* dst,
                                           int srcSize) {
  int srcPos;
  int groupHeaderPos;
  int dstPos;
  u8 groupHeaderBitRaw;
  EGGContext ctx{};

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
    findMatch(&ctx, src, srcPos, srcSize, &matchOffset, &firstMatchLen);
    if (firstMatchLen > 2) {
      int secondMatchOffset;
      int secondMatchLen;
      findMatch(&ctx, src, srcPos + 1, srcSize, &secondMatchOffset,
                &secondMatchLen);
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

static inline void findMatch(EGGContext* ctx, const u8* src, int srcPos,
                             int maxSize, int* matchOffset, int* matchSize) {
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
         (windowOffset =
              searchWindow(ctx, &src[srcPos], windowSize, &src[window],
                           srcPos + windowSize - window)) < srcPos - window) {
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

#if 1
static int searchWindow(EGGContext* ctx, const u8* needle, int needleSize,
                        const u8* haystack, int haystackSize) {
  int itHaystack; // r8
  int itNeedle;   // r9

  if (needleSize > haystackSize)
    return haystackSize;
  computeSkipTable(ctx, needle, needleSize);

  // Scan forwards for the last character in the needle
  for (itHaystack = needleSize - 1;;) {
    while (1) {
      if (needle[needleSize - 1] == haystack[itHaystack])
        break;
      itHaystack += ctx->sSkipTable[haystack[itHaystack]];
    }
    --itHaystack;
    itNeedle = needleSize - 2;
    break;
  Difference:
    // The entire needle was not found, continue search
    int skip = ctx->sSkipTable[haystack[itHaystack]];
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
#else
static int searchWindow(EGGContext* ctx, const u8* needle, int needleSize,
                        const u8* haystack, int haystackSize) {
  int itHaystack; // r8
  int itNeedle;   // r9

  if (needleSize > haystackSize)
    return haystackSize;
  computeSkipTable(ctx, needle, needleSize);

  // Scan forwards for the last character in the needle
  itHaystack = needleSize - 1;
  while (true) {
    while (needle[needleSize - 1] != haystack[itHaystack]) {
      itHaystack += ctx->sSkipTable[haystack[itHaystack]];
    }
    --itHaystack;
    itNeedle = needleSize - 2;

    // Scan backwards for the first difference
    int remainingBytes = itNeedle;
    int j;
    for (j = 0; j <= remainingBytes; ++j) {
      if (haystack[itHaystack] != needle[itNeedle]) {
        break;
      }
      --itHaystack;
      if (itNeedle == 0) {
        //printf("Delta: %i\n", (remainingBytes - j));
        continue;
	  }
      assert(itNeedle != 0);
      --itNeedle;
    }

    if (j > remainingBytes) {
      // The entire needle was found
      return itHaystack + 1;
    }

    // The entire needle was not found, continue search
    int skip = ctx->sSkipTable[haystack[itHaystack]];
    if (needleSize - itNeedle > skip) {
      skip = needleSize - itNeedle;
    }
    itHaystack += skip;
  }
}
#endif

static void computeSkipTable(EGGContext* ctx, const u8* needle,
                             int needleSize) {
  for (int i = 0; i < 256; ++i) {
    ctx->sSkipTable[i] = needleSize;
  }
  for (int i = 0; i < needleSize; ++i) {
    ctx->sSkipTable[needle[i]] = needleSize - i - 1;
  }
}

} // namespace rlibrii::szs
