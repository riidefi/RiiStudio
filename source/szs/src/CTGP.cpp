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

static std::optional<int> HandleNewInputByte(Yaz_file_struct* file, int value) {
  // Gather lookahead
  u32 copyLen = file->lookAheadBytes + 1;
  int iVar7 = file->bytesProcessed;
  u32 uVar17 = iVar7 + 1;
  file->bytesProcessed++;
  file->lookAheadBytes = copyLen;
  file->lookAhead = value << 0x18 | file->lookAhead >> 0x8;
  if (copyLen < MIN_SZS_RUNLENGTH) { // = 3
    return value;
  }
  bool bVar1;
  u32 uVar3;
  u32 hh;
  short sVar8;
  u32 uVar9;
  u32 uVar11;
  u32 uVar12;
  u16* puVar13;
  u16* puVar14;
  u16* puVar15;
  u32 uVar16;
  u16 res;

  // Once we have exhausted the window we need to refresh it
  if (iVar7 > 4100) {
    uVar9 = file->hashVacancies;
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
        goto LAB_807eb550;
      }
      sVar8 = sVar8 + 0x1;
      iVar7 = iVar7 + -0x1;
    } while (iVar7 != 0x0);
    uVar9 = uVar9 + 0x1;
    file->hashMap[uVar11] = 0x8000;
  LAB_807eb550:
    file->hashVacancies = uVar9;
    
    // "de-vacuum" hash table
    if (file->hashVacancies > 0x555) {
      memset(&file->hashMeta, 0xff, 0x8000);
      copyLen = 0x0;
      puVar13 = file->hashMap;
      do {
        if (((*puVar13 & 0x8000) != 0x0) &&
            (uVar17 = copyLen, (*puVar13 & 0x4000) == 0x0)) {
          do {
            uVar17 = uVar17 + 1 & 0x3fff;
            puVar14 = puVar13;
            uVar11 = copyLen;
          } while ((short)file->hashMap[uVar17] < 0x0);
          do {
            *puVar14 = 0x0;
            file->hashMeta[uVar11] = 0xffff;
            uVar9 = uVar17;
            do {
              do {
                uVar9 = (uVar9 - 1) % HASH_MAP_SIZE;
                if (uVar11 == uVar9) {
                  assert(*puVar14 == 0x0);
                  goto LAB_807eb594;
                }
                res = file->hashMap[uVar9];
                puVar15 = file->hashMap + uVar9;
                assert((res & 0x8000) != 0x0);
              } while ((res & 0x4000) == 0x0);
              hh = file->hashMeta[uVar9];
              if (hh == 0xffff) {
                hh = hash1(
                    (u32)file->window[(res + 1) % WINDOW_SIZE] << 0x8 |
                    (u32)file->window[(res + 2) % WINDOW_SIZE] << 0x10 |
                    (u32)file->window[res % WINDOW_SIZE]);
                hh = hh & 0xffff;
                file->hashMeta[uVar9] = (short)hh;
              }
              assert(hh < HASH_MAP_SIZE);
            } while (((uVar9 - hh) % HASH_MAP_SIZE) <
                     ((uVar11 - hh) % HASH_MAP_SIZE));
            *puVar14 = *puVar15;
            file->hashMeta[uVar11] = file->hashMeta[uVar9];
            *puVar15 = 0x8000;
            file->hashMeta[uVar9] = 0xffff;
            puVar14 = puVar15;
            uVar11 = uVar9;
            assert((*puVar15 & 0x4000) == 0);
          } while (true);
        }
      LAB_807eb594:
        bVar1 = copyLen != 0x3fff;
        puVar13 = puVar13 + 1;
        copyLen = copyLen + 1;
      } while (bVar1);
      uVar17 = file->bytesProcessed;
      file->hashVacancies = 0;
    }
  }
  // Handle other triple
  if (0x9 < uVar17) {
    uVar9 = file->hashVacancies;
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
        goto LAB_807eb78c;
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
    uVar9 = uVar9 - (res >> 0xf);
  LAB_807eb78c:
    file->hashVacancies = uVar9;
  }
  iVar7 = file->matchPos;
  if (iVar7 != -1) {
    // Try to extend match
    if ((file->lookAhead >> 0x8 & 0xff) == (u32)file->window[iVar7]) {
      copyLen = file->windowPos;
      file->window[copyLen] = (u8)(file->lookAhead >> 0x8);
      copyLen = copyLen + 1 & 0xfff;
      file->windowPos = copyLen;
      file->lookAheadBytes = file->lookAheadBytes - 1;
      uVar17 = iVar7 + 1U & 0xfff;
      file->matchPos = uVar17;
      file->matchLen = file->matchLen + 1;
      assert(file->matchDist == ((copyLen - 1) - uVar17 & 0xfff));
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
