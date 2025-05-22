#include <functional>

#include "SZS.hpp"

#include <assert.h>
#include <string.h>

#include <optional>

// Adapted from work from wit and Palapeli

#define WINDOW_SIZE 0x1000
#define HASH_MAP_SIZE 0x4000

#define MIN_SZS_RUNLENGTH 3
#define MAX_SZS_SMALL_RUNLENGTH 0x11
#define MAX_SZS_RUNLENGTH 0x111

using WRITE_FN =
    std::function<size_t(const void* buffer, size_t size, size_t count)>;

struct Yaz_file_struct {
  u16 hashMeta[HASH_MAP_SIZE];

  u16 getHash(u32 index) const {
    assert(index < HASH_MAP_SIZE);
    return hashMap[index];
  }

  void writeDoubleToCodeBuf(u32 upper4Bits, u32 lower12Bits) {
    codeBuffer[codeBufferLocation] = (upper4Bits << 4) | (lower12Bits >> 0x8);
    codeBuffer[(codeBufferLocation + 1) % 256] = (u8)lower12Bits;
    codeBufferLocation += 2;
    codeBufferIndex -= 1;
  }
  void writeTripToCodeBuf(u32 first16Bits, u32 second8Bits) {
    codeBuffer[codeBufferLocation] = (first16Bits >> 0x8);
    codeBuffer[(codeBufferLocation + 1) % 256] = (u8)first16Bits;
    codeBuffer[(codeBufferLocation + 2) % 256] = second8Bits;
    codeBufferLocation += 3;
    codeBufferIndex -= 1;
  }

  u32 writeSzsMatchToCodeBufWithTruncation(u32 matchDist, u32 copyLen) {
    if (copyLen <= MAX_SZS_SMALL_RUNLENGTH) {
      writeDoubleToCodeBuf((char)copyLen - 2, matchDist);
      return copyLen;
    }

    if (copyLen > MAX_SZS_RUNLENGTH) {
      copyLen = MAX_SZS_RUNLENGTH;
    }
    writeTripToCodeBuf(matchDist, (char)copyLen - 18);
    return copyLen;
  }

  void writeSzsDirectByteCopyToCodeBuf(u8 val) {
    codeBuffer[codeBufferLocation] = val;
    codeBufferLocation += 1;
    codeBufferIndex -= 1;

    codeBuffer[0] |= (u8)(1 << codeBufferIndex);
  }

  void pushWindow(u8 val) {
    window[windowPos] = val;
    windowPos = (windowPos + 1) % WINDOW_SIZE;
  }

  WRITE_FN file_write;
  u32 _0x4;
  int bytesProcessed;
  u32 windowPos; // C
  u8 window[WINDOW_SIZE];
  u32 lookAhead;
  u32 lookAheadBytes;
  u32 hashVacancies;
  u16 hashMap[HASH_MAP_SIZE]; // 101c
  int matchPos;           // 901c
  u32 matchDist;           // 9020
  u32 matchLen;
  u32 outputBufferSizeMaybe;
  u8 field13_0x902c[0x922C - 0x902C];
  u8 codeBufferLocation; // 922c
  u8 codeBufferIndex;    // 922d
  u8 codeBuffer[256];
};

int Yaz_fputc_r(int value, Yaz_file_struct* file);
bool Yaz_open(Yaz_file_struct* file, WRITE_FN fptr);

namespace rlibrii::szs {
Result<std::vector<u8>> encodeCTGP(std::span<const u8> buf) {
  std::vector<u8> result;
  WRITE_FN write_ = [&](const void* buffer, size_t size, size_t count) {
    size_t len = size * count;
    size_t pos = result.size();
    result.resize(pos + len);
    memcpy(&result[pos], buffer, len);
    return count;
  };
  Yaz_file_struct stream;
  memset(&stream, 0, sizeof(stream));

  if (!Yaz_open(&stream, write_)) {
    return tl::unexpected("encodeCTGP: Yaz_open failed");
  }
  for (u8 c : buf) {
    if (Yaz_fputc_r(c, &stream) != c) {
      return tl::unexpected("encodeCTGP: Yaz_fputc_r failed");
    }
  }
  Yaz_fputc_r(-1, &stream);
  result[4] = stream.bytesProcessed >> 24;
  result[5] = stream.bytesProcessed >> 16;
  result[6] = stream.bytesProcessed >> 8;
  result[7] = stream.bytesProcessed;

  // Validate
  {
    if (getExpandedSize(result).value_or(0) != buf.size()) {
      return tl::unexpected("encodeCTGP: Failed to produce a valid SZS header");
    }
    std::vector<u8> out(buf.size());
    if (!decode(out, result)) {
      return tl::unexpected("encodeCTGP: Failed to produce a valid SZS stream");
    }
    if (memcmp(out.data(), buf.data(), out.size())) {
      return tl::unexpected("encodeCTGP: Produced a corrupt file");
    }
  }
  return result;
}
} // namespace rlibrii::szs

u32 hash1(u32 value) {
  assert(!(value & 0xff000000));

  return ((value * value * 0xEF34 + value + 0xB205) >> 10) &
         (HASH_MAP_SIZE - 1);
}

int Yaz_buffer_putcode_r(Yaz_file_struct* file) {
  file->file_write(file->codeBuffer, file->codeBufferLocation, 1);
  return 0;
}

static int WriteMatchToFile(Yaz_file_struct* file) {
  // TODO: Should this be while(copyLen > 3)?
  if (file->matchLen > 3) {
    while (file->matchLen > 2) {
      if (file->codeBufferIndex == 0) {
        file->codeBufferIndex = 8;
        file->codeBuffer[0x0] = 0;
        file->codeBufferLocation = 1;
      }
      // 12-bit
      assert(file->matchDist < WINDOW_SIZE);
      u32 written = file->writeSzsMatchToCodeBufWithTruncation(
          file->matchDist, file->matchLen);
      if ((file->codeBufferIndex == 0) && Yaz_buffer_putcode_r(file)) {
        return -1;
      }
      file->matchLen -= written;
    }
  }
  return 0;
}

/*****************************************************************************
 * Compact ("vacuum") the open‑addressed hash table used by the Yaz‑style
 * LZ77/LZSS encoder.  It erases tomb‑stones and shifts live entries left so
 * that probe sequences remain contiguous (Robin‑Hood deletion).
 *
 *  ─── Bit layout of each 16‑bit slot ──────────────────────────────────────
 *  15 : EVER_USED   – set once the bucket has been touched (0x8000)
 *  14 : TOMBSTONE   – set when the element was deleted      (0x4000)
 *  13‑0 : WINDOW_POS – 14‑bit offset into the sliding window
 *
 *  A bucket is:
 *     • *empty*        : EVER_USED == 0
 *     • *live* element : EVER_USED == 1 && TOMBSTONE == 0
 *     • *tomb‑stone*   : EVER_USED == 1 && TOMBSTONE == 1
 ****************************************************************************/
static inline bool isEverUsed(uint16_t v)    { return v & 0x8000; }
static inline bool isTombstone(uint16_t v)   { return v & 0x4000; }
static inline bool isLive(uint16_t v)        { return isEverUsed(v) && !isTombstone(v); }


static inline uint16_t homeBucket(const Yaz_file_struct* f, uint16_t entry) {
    /* Re‑compute the canonical bucket (hash) of the 3‑byte sequence whose
       first byte lives at window[ pos ].  The table caches this in
       file->hashMeta the first time it needs it. */
    uint32_t pos = entry & 0x3fff;                       // original offset
    uint32_t key =
          (uint32_t)f->window[ pos                      ]        |
          (uint32_t)f->window[(pos + 1) & (WINDOW_SIZE - 1)] <<  8 |
          (uint32_t)f->window[(pos + 2) & (WINDOW_SIZE - 1)] << 16;

    return uint16_t(hash1(key) & 0xffff);
}

static void devacuumHashTable(Yaz_file_struct* file) {
  /* 1.  Bail out unless the tomb‑stone count is high enough. */
  if (file->hashVacancies <= 0x555)
    return;

  /* 2.  Mark every entry in the metadata cache as "unknown." */
  memset(&file->hashMeta, 0xff, 0x8000);

  u32 uVar9;
  u32 uVar11;
  u16* puVar14;
  u16* puVar15;
  u16 res;
  u32 hh;
  
  /* 3.  Sweep each bucket.  Whenever we encounter the head of a probe
           cluster, we compress it by repeatedly pulling live elements
           leftward into the nearest hole. */
  for (u32 i = 0; i < 0x4000; ++i) {
    /* Only start vacuumming from the first live element in a cluster. */
    if (!isEverUsed(file->hashMap[i]) || isTombstone(file->hashMap[i])) {
      continue;
    }

    u32 hole = i;

    /* ------------------------------------------------------------------
    * Step A – find the first truly empty slot (EVER_USED == 0) after
    *           this live element.  That becomes our movable "hole."
    * ----------------------------------------------------------------*/
    do {
      hole = hole + 1 & 0x3fff;
      puVar14 = file->hashMap + i;
      uVar11 = i;
    } while ((short)file->hashMap[hole] < 0x0);
    
    /* ------------------------------------------------------------------
        * Step B – walk *backwards* through the cluster, pulling any element
        *           that may legally move into the hole (Robin‑Hood rule).
        * ----------------------------------------------------------------*/
    do {
      /* Walk left until we find a live entry; tomb‑stones are skipped. */
      *puVar14 = 0x0;
      file->hashMeta[uVar11] = 0xffff;
      uVar9 = hole;
      do {
        do {
          uVar9 = (uVar9 - 1) % HASH_MAP_SIZE;
          if (uVar11 == uVar9) { // cluster fully compacted
            assert(*puVar14 == 0x0);
            goto CONTINUE_LOOP;
          }
          res = file->hashMap[uVar9];
          puVar15 = file->hashMap + uVar9;
          assert(isEverUsed(res));
        } while (!isTombstone(res));
        
        /* Compute / fetch the element's home bucket (canonical hash). */
        hh = file->hashMeta[uVar9];
        if (hh == 0xffff) {
          hh = homeBucket(file, res);
          file->hashMeta[uVar9] = (short)hh;
        }
        assert(hh < HASH_MAP_SIZE);
      /* If moving the element would *increase* its probe distance,
            we must stop--the invariant of non‑decreasing distances
            would be violated. */
      } while (((uVar9 - hh) % HASH_MAP_SIZE) <
                ((uVar11 - hh) % HASH_MAP_SIZE));
      /* Move the entry one position forward (into the hole). */
      *puVar14 = *puVar15;
      file->hashMeta[uVar11] = file->hashMeta[uVar9];
      /* Leave an "ever used but empty" marker behind. */
      *puVar15 = 0x8000; // EVER_USED, not TOMBSTONE
      file->hashMeta[uVar9] = 0xffff;
      puVar14 = puVar15;  // new hole sits one step left
      uVar11 = uVar9;
      assert(!isTombstone(*puVar15));
    } while (true);
  CONTINUE_LOOP:
    ;
  }

  /* 4.  All tomb-stones eliminated--reset the counter. */
  file->hashVacancies = 0;
}

static void refreshWindow(Yaz_file_struct* file) {
  u32 copyLen;
  u32 hh;
  u32 uVar11;
  int iVar7;
  u16 res;
  short sVar8;
  u32 uVar16;
  u32 uVar12;
  u32 uVar3;

  copyLen = file->windowPos + 0x1 & 0xfff;
  hh = (u32)file->window[copyLen + 0x1 & 0xfff] << 0x8 |
        (u32)file->window[copyLen + 0x2 & 0xfff] << 0x10 |
        (u32)file->window[copyLen];
  uVar11 = hash1(hh);
  assert(uVar11 <= 0x3fff);
  iVar7 = 0x4001;
  res = file->hashMap[uVar11];
  while (true) {
    assert((res & 0x8000) != 0);
    if (((res & 0x4000) != 0x0) && (copyLen == (res & 0x3fff)))
      break;
    iVar7 = iVar7 + -1;
    assert(iVar7 != 0);
    uVar11 = uVar11 + 1 & 0x3fff;
    res = file->hashMap[uVar11];
  }
  sVar8 = 1;
  iVar7 = 0x4;
  uVar16 = copyLen;
  do {
    uVar12 = uVar16 + 0x2;
    uVar3 = uVar16 + 0x3;
    uVar16 = uVar16 + 0x1;
    if (hh == ((u32)file->window[uVar12 & 0xfff] << 0x8 |
                (u32)file->window[uVar3 & 0xfff] << 0x10 |
                (u32)file->window[uVar16 & 0xfff])) {
      file->hashMap[uVar11] = ((short)copyLen + sVar8 & 0xfffU) | 0xc000;
      return;
    }
    sVar8 = sVar8 + 0x1;
    iVar7 = iVar7 + -0x1;
  } while (iVar7 != 0x0);
  file->hashVacancies++;
  file->hashMap[uVar11] = 0x8000;
}

static void handleOtherTriple(Yaz_file_struct* file) {
  u32 copyLen;
  u32 uVar11;
  int iVar7;
  u32 uVar17;
  u16 res;
  
  copyLen = file->windowPos - 3 & 0xfff;
  uVar11 = (u32)file->window[(copyLen + 1) % WINDOW_SIZE] << 0x8 |
            (u32)file->window[(copyLen + 2) % WINDOW_SIZE] << 0x10 |
            (u32)file->window[copyLen];
  iVar7 = 0x4;
  uVar17 = copyLen;
  do {
    if (uVar11 == ((u32)file->window[uVar17 - 0x3 & 0xfff] << 0x8 |
                    (u32)file->window[uVar17 - 0x2 & 0xfff] << 0x10 |
                    (u32)file->window[uVar17 - 0x4 & 0xfff]))
      return;
    uVar17 = uVar17 + 1;
    iVar7 = iVar7 + -1;
  } while (iVar7 != 0x0);
  uVar17 = hash1(uVar11);
  assert(uVar17 < HASH_MAP_SIZE);
  res = file->hashMap[uVar17];
  if ((res & 0x4000) != 0x0) {
    iVar7 = 0x4000;
    while (true) {
      uVar17 = uVar17 + 1 & 0x3fff;
      res = file->hashMap[uVar17];
      if ((res & 0x4000) == 0x0)
        break;
      iVar7 = iVar7 + -1;
      assert(iVar7 != 0);
    }
  }
  file->hashMap[uVar17] = (u16)copyLen | 0xc000;
  file->hashVacancies = file->hashVacancies - (res >> 0xf);
}

static std::optional<int> HandleNewInputByte(Yaz_file_struct* file, int value) {
  // Gather lookahead
  int old_bytes_processed = file->bytesProcessed;
  file->bytesProcessed++;
  file->lookAheadBytes = file->lookAheadBytes + 1;
  file->lookAhead = value << 0x18 | file->lookAhead >> 0x8;
  if (file->lookAheadBytes < MIN_SZS_RUNLENGTH) { // = 3
    return value;
  }

  // Once we have exhausted the window we need to refresh it
  if (old_bytes_processed > 4100) {
    refreshWindow(file);
    // "de-vacuum" hash table
    devacuumHashTable(file);
  }
  // Handle other triple
  if (file->bytesProcessed > 9) {
    handleOtherTriple(file);
  }
  if (file->matchPos != -1) {
    // Try to extend match
    if ((file->lookAhead >> 0x8 & 0xff) == (u32)file->window[file->matchPos]) {
      file->window[file->windowPos] = (u8)(file->lookAhead >> 0x8);
      file->windowPos = file->windowPos + 1 & 0xfff;
      file->lookAheadBytes = file->lookAheadBytes - 1;
      file->matchPos = (u32)(file->matchPos + 1U & 0xfff);
      file->matchLen = file->matchLen + 1;
      assert(file->matchDist == ((file->windowPos - 1) - file->matchPos & 0xfff));
      return value;
    }
    if (WriteMatchToFile(file) != 0) {
      return -1;
    }
  }
  return std::nullopt;
}

static std::optional<int> HandleResidue(Yaz_file_struct* file) {
  while (file->matchLen > 0) {
    while (true) {
      assert(file->matchPos != -1);
      if (file->codeBufferIndex == 0) {
        file->codeBuffer[0] = 0;
        file->codeBuffer[1] =
            file->window[(file->windowPos - file->matchLen) %
                             WINDOW_SIZE];
        file->codeBufferLocation = 2;
        file->codeBufferIndex = 7;
        
      } else {
        file->codeBuffer[file->codeBufferLocation] =
            file->window[(file->windowPos - file->matchLen) %
                             WINDOW_SIZE];
        file->codeBufferLocation++;
        file->codeBufferIndex--;
      }
      file->codeBuffer[0] |= (1 << file->codeBufferIndex);
      file->matchLen--;
      if (file->codeBufferIndex != 0)
        break;
      if (Yaz_buffer_putcode_r(file) != 0) {
        return -1;
      }
      if (file->matchLen == 0)
        return std::nullopt;
    }
  }

  return std::nullopt;
}

static int FinalizeSimple(Yaz_file_struct* file, int value) {
  int remainingBytes;
  int bitShift;

  while (true) {
    bitShift = file->lookAheadBytes * -8 + 0x20;
    bool loop_again = false;
    for (remainingBytes = file->lookAheadBytes; remainingBytes != 0; --remainingBytes) {
      file->pushWindow(file->lookAhead >> bitShift);
      bitShift += 8;
      if (file->codeBufferIndex == 0) {
        file->codeBufferIndex = 8;
        file->codeBufferLocation = 1;
        file->codeBuffer[0] = 0;
      }
      file->writeSzsDirectByteCopyToCodeBuf(
          file->lookAhead >> file->lookAheadBytes * (-8 + 0x20));
      file->lookAheadBytes -= 1;
      if (file->codeBufferIndex == 0) {
        if (Yaz_buffer_putcode_r(file) != 0) {
          return value;
        }
        loop_again = true;
        break;
      }
    }
    if (!loop_again) {
      break;
    }
  }
  if (file->codeBufferIndex != 0 && Yaz_buffer_putcode_r(file)) {
    return -1;
  }
  if (file->outputBufferSizeMaybe == 0) {
    return 0;
  }
  if (file->file_write(file->field13_0x902c, 1, file->outputBufferSizeMaybe) ==
      file->outputBufferSizeMaybe) {
    return 0;
  }
  return -1;
}

static int FinalizeComplex(Yaz_file_struct* file, int value) {
  u32 copyLen;
  u8 bVar4;
  u32 hh;
  u8 bVar5;
  u8 uVar6;
  int iVar7;
  u32 uVar9;
  u32 uVar11;
  u32 uVar17;

  assert(file->lookAheadBytes == 3);

  uVar17 = file->lookAhead;
  copyLen = file->windowPos;
  uVar6 = (u8)(uVar17 >> 8);
  file->window[copyLen] = uVar6;
  copyLen = (copyLen + 1) % WINDOW_SIZE;
  file->windowPos = copyLen;
  file->lookAheadBytes = 2;

  if ((u32)file->bytesProcessed < 3) {
  LAB_807eb248:
    if (file->codeBufferIndex == 0) {
      file->codeBuffer[1] = uVar6;
      file->codeBufferLocation = 2;
      file->codeBufferIndex = 7;
      file->codeBuffer[0] = 0x80;
    } else {
      bVar5 = file->codeBufferLocation;
      bVar4 = file->codeBufferIndex - 1;
      file->codeBuffer[bVar5] = uVar6;
      file->codeBufferLocation = bVar5 + 1;
      file->codeBufferIndex = bVar4;
      file->codeBuffer[0] |= (u8)(1 << bVar4);

      if (bVar4 == 0) {
        iVar7 = Yaz_buffer_putcode_r(file);
        if (iVar7 != 0) {
          return -1;
        }
        copyLen = file->windowPos;
        uVar17 = file->lookAhead;
      }
    }

    file->matchLen = 0;
    file->matchDist = 0;
    uVar11 = (copyLen - 1) % WINDOW_SIZE;
    file->matchPos = -1;
    uVar17 = (uVar17 >> 16) & 0xff;

    if (file->window[uVar11] == uVar17) {
      file->matchPos = uVar11;
    }

    copyLen = (copyLen - 2) % WINDOW_SIZE;
    if (uVar17 == file->window[copyLen]) {
      file->matchPos = copyLen;
      file->matchDist = 1;
    }
  } else {
    uVar11 = hash1(uVar17 >> 8);
    iVar7 = 0x4001;
    assert(uVar11 < HASH_MAP_SIZE);
    uVar9 = (u32)file->hashMap[uVar11];

    if ((file->hashMap[uVar11] & 0x4000) == 0) {
      goto LAB_807eb408;
    }

    while (true) {
      assert((uVar9 & 0x8000) != 0);
      hh = uVar9 % WINDOW_SIZE;

      if (uVar17 >> 8 ==
          ((u32)file->window[(uVar9 + 1) % WINDOW_SIZE] << 8 |
           (u32)file->window[(uVar9 + 2) % WINDOW_SIZE] << 16 |
           (u32)file->window[hh])) {
        break;
      }

      while (true) {
        iVar7--;
        assert(iVar7 != 0);
        uVar11 = (uVar11 + 1) % HASH_MAP_SIZE;
        uVar9 = (u32)file->hashMap[uVar11];

        if ((file->hashMap[uVar11] & 0x4000) != 0) {
          break;
        }

      LAB_807eb408:
        if ((uVar9 & 0x8000) == 0) {
          goto LAB_807eb248;
        }
      }
    }

    uVar17 = (copyLen - 1) % WINDOW_SIZE;
    assert(file->window[uVar17] == file->window[hh]);
    assert(uVar17 != hh);

    uVar17 = (hh + 1) % WINDOW_SIZE;
    file->matchLen = 1;
    file->matchPos = uVar17;
    file->matchDist = ((copyLen - 1) - uVar17) % WINDOW_SIZE;
  }

  return value;
}

int Yaz_fputc_r(int value, Yaz_file_struct* file) {
  if (value == -1) {
    if (file->matchPos != -1) {
      if (WriteMatchToFile(file) != 0) {
        return -1;
      }
    }
  } else {
    auto res = HandleNewInputByte(file, value);
    if (res.has_value()) {
      return *res;
    }
  }

  auto t = HandleResidue(file);

  if (t.has_value()) {
    return *t;
  }

  if (value == -1) {
    return FinalizeSimple(file, value);
  } else {
    return FinalizeComplex(file, value);
  }
}

bool Yaz_open(Yaz_file_struct* file, WRITE_FN fptr) {
  assert(file != nullptr);
  assert(fptr != nullptr);
  memset(file->hashMeta, 0xff, sizeof(file->hashMeta));
  memset(file, 0, sizeof(*file));

  file->file_write = fptr;
  if (!file->file_write)
    return false;

  const u8 szsHeader[] = {
      0x59, 0x61, 0x7A, 0x30, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  [[maybe_unused]] int ret = file->file_write(szsHeader, sizeof(szsHeader), 1);
  assert(ret == 1);

  file->matchPos = -1;

  return true;
}
