#pragma once

#include "util.h"

#include "expected.hpp"
#include <span>
#include <vector>

#include "SZS.hpp"

#include <assert.h>

struct SZPData {
  u32 decompressedSize = 0;
  std::vector<u8> byteChunkAndCountModifiersTable;
  std::vector<u16> linkTable;
  std::vector<bool> bitstream;
};

static inline std::vector<u32> CreateU32Stream(std::vector<bool> bitstream) {
  std::vector<u32> u32stream;
  u32 counter = 0;
  u32 tmp = 0;
  for (size_t i = 0; i < bitstream.size(); ++i) {
    tmp |= bitstream[i] ? (0x80000000 >> counter) : 0;
    ++counter;
    if (counter == 32 || i == bitstream.size() - 1) {
      counter = 0;
      u32stream.push_back(tmp);
      tmp = 0;
    }
  }
  return u32stream;
}

static inline std::vector<u8> WriteSZPData(const SZPData& szp) {
  std::vector<u32> u32stream = CreateU32Stream(szp.bitstream);

  const u32 u32DataOffset = 16; // Header size
  const u32 u16DataOffset = u32DataOffset + u32stream.size() * 4;
  const u32 u8DataOffset = u16DataOffset + szp.linkTable.size() * 2;
  const u32 fileEnd = u8DataOffset + szp.byteChunkAndCountModifiersTable.size();

  std::vector<u8> result(fileEnd);
  result[0] = 'Y';
  result[1] = 'a';
  result[2] = 'y';
  result[3] = '0';
  result[4] = (szp.decompressedSize & 0xff00'0000) >> 24;
  result[5] = (szp.decompressedSize & 0x00ff'0000) >> 16;
  result[6] = (szp.decompressedSize & 0x0000'ff00) >> 8;
  result[7] = (szp.decompressedSize & 0x0000'00ff) >> 0;
  result[8] = (u16DataOffset & 0xff00'0000) >> 24;
  result[9] = (u16DataOffset & 0x00ff'0000) >> 16;
  result[0xA] = (u16DataOffset & 0x0000'ff00) >> 8;
  result[0xB] = (u16DataOffset & 0x0000'00ff) >> 0;
  result[0xC] = (u8DataOffset & 0xff00'0000) >> 24;
  result[0xD] = (u8DataOffset & 0x00ff'0000) >> 16;
  result[0xE] = (u8DataOffset & 0x0000'ff00) >> 8;
  result[0xF] = (u8DataOffset & 0x0000'00ff) >> 0;

  {
    u32 it = u32DataOffset;
    for (u32 u : u32stream) {
      result[it++] = (u & 0xff00'0000) >> 24;
      result[it++] = (u & 0x00ff'0000) >> 16;
      result[it++] = (u & 0x0000'ff00) >> 8;
      result[it++] = (u & 0x0000'00ff) >> 0;
    }
    assert(it == u16DataOffset);
  }
  {
    u32 it = u16DataOffset;
    for (u16 u : szp.linkTable) {
      result[it++] = (u & 0x0000'ff00) >> 8;
      result[it++] = (u & 0x0000'00ff) >> 0;
    }
    assert(it == u8DataOffset);
  }
  memcpy(result.data() + u8DataOffset,
         szp.byteChunkAndCountModifiersTable.data(),
         szp.byteChunkAndCountModifiersTable.size());
  return result;
}

// Copy paste
static inline tl::expected<u32, std::string>
getExpandedSize_Copy(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return tl::unexpected("File too small to be a YAZ0 file");

  if (!(src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0')) {
    return tl::unexpected("Data is not a YAZ0 file");
  }
  return (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
}

static inline u32 SZPToSZP_UpperBoundSize(u32 szsSize) {
  // Worst case, the u8stream -> u32stream conversion adds 3 padding bytes?
  return szsSize + 3;
}

// De-interlace
static inline tl::expected<std::vector<u8>, std::string>
SZSToSZP(std::span<const u8> src) {
  auto exp = getExpandedSize_Copy(src);
  if (!exp) {
    return tl::unexpected("Source is not a SZS compressed file!");
  }

  SZPData result;
  result.decompressedSize = *exp;

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

  const auto read_group = [&](bool raw) {
    result.bitstream.push_back(raw);
    if (raw) {
      result.byteChunkAndCountModifiersTable.push_back(take8());
      ++out_position;
      return;
    }

    const u16 group = take16();
    const int reverse = (group & 0xfff) + 1;
    const int g_size = group >> 12;
    result.linkTable.push_back(group);

    u8 next = 0;
    if (g_size == 0) {
      next = take8();
      result.byteChunkAndCountModifiersTable.push_back(next);
    }
    int size = g_size ? g_size + 2 : next + 18;

    for (int i = 0; i < size; ++i) {
      ++out_position;
    }
  };

  const auto read_chunk = [&]() {
    const u8 header = take8();

    for (int i = 0; i < 8; ++i) {
      if (in_position >= src.size() || out_position >= result.decompressedSize)
        return;
      read_group(header & (1 << (7 - i)));
    }
  };

  while (in_position < src.size() && out_position < result.decompressedSize)
    read_chunk();

  return WriteSZPData(result);
}

static inline tl::expected<u32, std::string>
SZSToSZP_C(std::span<u8> dst, std::span<const u8> src) {
  auto szp = SZSToSZP(src);
  if (!szp) {
    return tl::unexpected(szp.error());
  }
  if (dst.size() < szp->size()) {
    return tl::unexpected("Dst buffer was insufficiently sized! Expected " +
                          std::to_string(szp->size()) + " instead was given " +
                          std::to_string(dst.size()) + " bytes to work with.");
  }
  memcpy(dst.data(), szp->data(), szp->size());
  return static_cast<u32>(szp->size());
}
