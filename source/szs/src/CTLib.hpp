//////////////////////////////////////////////////
//  Copyright (c) 2020 Nara Hiero
//
// This file is licensed under the MIT License.
// Previously licensed under GPLv3. Re-licensed with permission.
//////////////////////////////////////////////////

//
// * Adapted to use std::vector, std::span over custom Buffer types
//

#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace CTLib {

static inline size_t calculateMaxSize(size_t baseSize) {
  return (baseSize + (baseSize >> 3) + 0x18) & ~0x7;
}

static inline void writeHeader(std::vector<uint8_t>& out, size_t len) {
  const char* formatString = "Yaz0";
  out.insert(out.begin(), formatString, formatString + 4);

  uint32_t length = static_cast<uint32_t>(len);
  out[4] = ((length >> 24) & 0xFF);
  out[5] = ((length >> 16) & 0xFF);
  out[6] = ((length >> 8) & 0xFF);
  out[7] = (length & 0xFF);
  out.insert(out.begin() + 8, 12, 0);
}

static inline size_t findBestMatch(uint8_t* curr, uint8_t* end, uint16_t* table,
                                   uint16_t tableOff, size_t& best) {
  size_t size = (end - curr) > 0x111 ? 0x111 : (end - curr);

  uint16_t* offs = table + (*curr << 13);
  uint16_t* offsEnd = offs + offs[0] + 1;

  uint16_t limit = 0x1000 + tableOff;
  while (++offs<offsEnd&& * offs> limit) {
    // skip out of range occurences
  }

  best = 0;
  uint8_t* bestLoc = curr;
  uint8_t* search;
  int16_t back;
  for (size_t i = 1; offs < offsEnd; ++offs, i = 1) {
    back = *offs - tableOff;
    if (back <= 0) {
      break; // all remaining offsets are after the current location
    }
    search = curr - back;
    while ((curr[i] == search[i]) && (++i < size)) {
      // loop until difference is found or max size is reached
    }
    if (best < i) {
      best = i;
      bestLoc = search;
    }
    if (best == size) {
      break; // maximum match length, no point in trying to find another
    }
  }

  return curr - bestLoc;
}

static inline void fillInTable(uint8_t* curr, uint8_t* end, uint16_t* table) {
  size_t size = (end - curr) > 0x1000 ? 0x1000 : (end - curr);
  for (uint16_t i = 0; i < size; ++i) {
    uint16_t count = ++table[curr[i] << 13];
    table[(curr[i] << 13) + count] = 0xFFF - i;
  }
}

static inline void updateTable(uint8_t* data, uint8_t* dataEnd, uint16_t* table,
                               uint16_t off) {
  uint16_t limit = 0x1000 - off;
  uint16_t off_push = 0xFFF + off;

  uint16_t *offs, *start, *curr, *end;
  for (size_t i = 0; i < 0x100; ++i) {
    offs = start = curr = table + (i << 13);
    end = offs + offs[0] + 1;

    while (++curr < end && *curr >= limit) {
      // find first valid offset
    }

    --curr;
    while (++curr < end) {
      *(++start) = *curr + off_push;
    }

    *offs -= static_cast<uint16_t>(curr - start - 1); // subtract from count
  }
  fillInTable(data, dataEnd, table);
}

static inline void compressData(std::span<const uint8_t> data,
                                std::vector<uint8_t>& out) {
  uint8_t* curr = const_cast<uint8_t*>(data.data());
  uint8_t* end = curr + data.size();
  uint8_t* outStart = const_cast<uint8_t*>(out.data() + 16);

  auto offsetsTableHandler = std::make_unique<uint16_t[]>(1 << 21);
  uint16_t* offsetsTable = offsetsTableHandler.get();
  int16_t tableOff = 0xFFF;

  for (size_t i = 0; i < 0x100; ++i) {
    offsetsTable[i << 13] = 0; // set offset counts to 0
  }
  fillInTable(curr, end, offsetsTable);

  uint8_t* groupHead = outStart;
  uint8_t groupIdx = 8;
  uint8_t* group = groupHead + 1;
  while (curr < end) {
    size_t size = 0;
    size_t pos = findBestMatch(curr, end, offsetsTable, tableOff, size) - 1;

    if (size > 2) // add back reference
    {
      if (size < 0x12) {
        *(group++) = static_cast<uint8_t>(((size - 0x2) << 4) | (pos >> 8));
        *(group++) = static_cast<uint8_t>(pos & 0xFF);
      } else // use three bytes chunk
      {
        *(group++) = static_cast<uint8_t>(pos >> 8);
        *(group++) = static_cast<uint8_t>(pos & 0xFF);
        *(group++) = static_cast<uint8_t>(size - 0x12);
      }

      curr += size;
      tableOff -= static_cast<int16_t>(size);
    } else // direct single byte copy
    {
      *(group++) = *(curr++);
      *groupHead |= 0x1; // set bit of current chunk

      --tableOff;
    }

    if (--groupIdx == 0) // move to next group
    {
      groupHead = group++;
      *groupHead = 0;
      groupIdx = 8;
    } else // move to next chunk
    {
      *groupHead <<= 1;
    }

    if (tableOff < 0) {
      updateTable(curr, end, offsetsTable, -tableOff);
      tableOff = 0xFFF;
    }
  }

  if (groupIdx < 8) // flush any remaining data
  {
    *groupHead <<= groupIdx - 1;
  }

  size_t len = group - outStart;
  if ((len & 0x3) > 0) // add padding
  {
    size_t padding = 4 - (len & 0x3);
    while (padding-- > 0) {
      *(group++) = 0x00;
    }
  }
  out.resize(16 + (group - outStart));
}

static inline std::vector<uint8_t> compressBase(std::span<const uint8_t> data) {
  std::vector<uint8_t> out(calculateMaxSize(data.size()));

  writeHeader(out, data.size());
  compressData(data, out);

  return out;
}

} // namespace CTLib
