#include <core/common.h>

#include <functional>

#include "SZS.hpp"

// Adapted from work from wit and Palapeli

#define BACK_BUF_SIZE 0x1000
#define HASH_MAP_SIZE 0x4000

using WRITE_FN =
    std::function<size_t(const void* buffer, size_t size, size_t count)>;

struct Yaz_file_struct {
  WRITE_FN file_write;
  u32 _0x4;
  int iteration;
  u32 backBufferLocation; // C
  u8 backBuffer[BACK_BUF_SIZE];
  u32 field5_0x1010;
  u32 byteShifterRemaining;
  u32 field7_0x1018;
  u16 hashMap[HASH_MAP_SIZE]; // 101c
  int copyLocation;           // 901c
  u32 copyDistance;           // 9020
  u32 copyLength;
  u32 field12_0x9028;
  u8 field13_0x902c[0x922C - 0x902C];
  u8 codeBufferLocation;          // 922c
  u8 codeBufferIndex;             // 922d
  u8 codeBuffer[0x9248 - 0x922E]; // not actual size probably
};

int Yaz_fputc_r(int* reent, int value, Yaz_file_struct* file);
bool Yaz_open(Yaz_file_struct* file, WRITE_FN fptr);

namespace librii::szs {
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
    return std::unexpected("encodeCTGP: Yaz_open failed");
  }
  for (u8 c : buf) {
    int reent = 0; // ?
    if (Yaz_fputc_r(&reent, c, &stream) != c) {
      return std::unexpected("encodeCTGP: Yaz_fputc_r failed");
    }
  }
  int reent;
  Yaz_fputc_r(&reent, -1, &stream);
  result[4] = stream.iteration >> 24;
  result[5] = stream.iteration >> 16;
  result[6] = stream.iteration >> 8;
  result[7] = stream.iteration;

  // Validate
  {
    if (getExpandedSize(result).value_or(0) != buf.size()) {
      return std::unexpected(
          "encodeCTGP: Failed to produce a valid SZS header");
    }
    std::vector<u8> out(buf.size());
    if (!decode(out, result)) {
      return std::unexpected(
          "encodeCTGP: Failed to produce a valid SZS stream");
    }
    if (memcmp(out.data(), buf.data(), out.size())) {
      return std::unexpected("encodeCTGP: Produced a corrupt file");
    }
  }
  return result;
}
} // namespace librii::szs

static u16 DAT_80ad1160[HASH_MAP_SIZE];

u32 hash1(u32 value) {
  assert(!(value & 0xff000000));

  return ((value * value * 0xEF34 + value + 0xB205) >> 10) &
         (HASH_MAP_SIZE - 1);
}

int Yaz_buffer_putcode_r(int* reent, u8* file2) {
  Yaz_file_struct* file = (Yaz_file_struct*)file2;

  file->file_write(file->codeBuffer, file->codeBufferLocation, 1);
  return 0;
}

#define ASSERT_FAIL(...) assert(false)

int Yaz_fputc_r(int* reent, int value, Yaz_file_struct* file) {
  bool bVar1;
  u32 uVar2;
  u8 bVar4;
  u32 uVar3;
  u32 hh;
  u8 bVar5;
  u8 uVar6;
  int iVar7;
  short sVar8;
  u32 uVar9;
  int iVar10;
  u32 uVar11;
  u32 uVar12;
  u16* puVar13;
  u16* puVar14;
  u16* puVar15;
  u32 uVar16;
  u32 uVar17;
  u16* puVar18;
  u16 res;

  if (value == -0x1) {
    if (file->copyLocation != -0x1) {
    LAB_807eaf78:
      uVar2 = file->copyLength;
      if (0x3 < uVar2) {
        if (file->codeBufferIndex == '\0') {
          file->codeBuffer[0x0] = '\0';
          file->codeBufferIndex = '\b';
          file->codeBufferLocation = '\x01';
        }
        do {
          if (file->codeBufferIndex == '\0') {
            file->codeBufferIndex = '\b';
            file->codeBuffer[0x0] = '\0';
            file->codeBufferLocation = '\x01';
          }
          uVar17 = file->copyDistance;
          if ((uVar17 & 0xfffff000) != 0x0) {
            // WARNING: Subroutine does not return
            ASSERT_FAIL();
          }
          bVar5 = (u8)(uVar17 >> 0x8);
          if (uVar2 < 0x112) {
            if (0x11 < uVar2)
              goto LAB_807eafb4;
            bVar4 = file->codeBufferLocation;
            file->codeBuffer[bVar4] = ((char)uVar2 + -0x2) * '\x10' | bVar5;
            file->codeBuffer[bVar4 + 0x1 & 0xff] = (u8)uVar17;
            file->codeBufferLocation = bVar4 + 0x2;
            uVar6 = file->codeBufferIndex + 0xff;
            file->codeBufferIndex = uVar6;
          } else {
            uVar2 = 0x111;
          LAB_807eafb4:
            bVar4 = file->codeBufferLocation;
            uVar11 = (u32)bVar4;
            file->codeBuffer[uVar11] = bVar5;
            file->codeBuffer[uVar11 + 0x1 & 0xff] = (u8)uVar17;
            file->codeBuffer[uVar11 + 0x2 & 0xff] = (char)uVar2 + 0xee;
            file->codeBufferLocation = bVar4 + 0x3;
            uVar6 = file->codeBufferIndex + 0xff;
            file->codeBufferIndex = uVar6;
          }
          if ((uVar6 == '\0') &&
              (iVar7 = Yaz_buffer_putcode_r(reent, (u8*)file), iVar7 != 0x0)) {
            return -0x1;
          }
          uVar2 = file->copyLength - uVar2;
          file->copyLength = uVar2;
        } while (0x2 < uVar2);
      }
      goto LAB_807eb1a8;
    }
  } else {
    uVar2 = file->byteShifterRemaining + 0x1;
    iVar7 = file->iteration;
    uVar17 = iVar7 + 0x1;
    file->iteration = uVar17;
    file->byteShifterRemaining = uVar2;
    file->field5_0x1010 = value << 0x18 | file->field5_0x1010 >> 0x8;
    if (uVar2 < 0x3) {
      return value;
    }
    if (0x6 < iVar7 + -0xffe) {
      puVar18 = file->hashMap;
      uVar9 = file->field7_0x1018;
      uVar2 = file->backBufferLocation + 0x1 & 0xfff;
      hh = (u32)file->backBuffer[uVar2 + 0x1 & 0xfff] << 0x8 |
           (u32)file->backBuffer[uVar2 + 0x2 & 0xfff] << 0x10 |
           (u32)file->backBuffer[uVar2];
      uVar11 = hash1(hh);
      if (0x3fff < uVar11) {
        // WARNING: Subroutine does not return
        ASSERT_FAIL();
      }
      iVar7 = 0x4001;
      res = file->hashMap[uVar11];
      while (true) {
        if ((res & 0x8000) == 0x0) {
          // WARNING: Subroutine does not return
          ASSERT_FAIL();
        }
        if (((res & 0x4000) != 0x0) && (uVar2 == (res & 0x3fff)))
          break;
        iVar7 = iVar7 + -0x1;
        if (iVar7 == 0x0) {
          // WARNING: Subroutine does not return
          ASSERT_FAIL();
        }
        uVar11 = uVar11 + 0x1 & 0x3fff;
        res = puVar18[uVar11];
      }
      sVar8 = 0x1;
      iVar7 = 0x4;
      uVar16 = uVar2;
      do {
        uVar12 = uVar16 + 0x2;
        uVar3 = uVar16 + 0x3;
        uVar16 = uVar16 + 0x1;
        if (hh == ((u32)file->backBuffer[uVar12 & 0xfff] << 0x8 |
                   (u32)file->backBuffer[uVar3 & 0xfff] << 0x10 |
                   (u32)file->backBuffer[uVar16 & 0xfff])) {
          file->hashMap[uVar11] = ((short)uVar2 + sVar8 & 0xfffU) | 0xc000;
          goto LAB_807eb550;
        }
        sVar8 = sVar8 + 0x1;
        iVar7 = iVar7 + -0x1;
      } while (iVar7 != 0x0);
      uVar9 = uVar9 + 0x1;
      file->hashMap[uVar11] = 0x8000;
    LAB_807eb550:
      file->field7_0x1018 = uVar9;
      if (0x555 < uVar9) {
        memset(&DAT_80ad1160, 0xff, 0x8000);
        uVar2 = 0x0;
        puVar13 = puVar18;
        do {
          if (((*puVar13 & 0x8000) != 0x0) &&
              (uVar17 = uVar2, (*puVar13 & 0x4000) == 0x0)) {
            do {
              uVar17 = uVar17 + 0x1 & 0x3fff;
              puVar14 = puVar13;
              uVar11 = uVar2;
            } while ((short)puVar18[uVar17] < 0x0);
            do {
              *puVar14 = 0x0;
              DAT_80ad1160[uVar11] = 0xffff;
              uVar9 = uVar17;
              do {
                do {
                  uVar9 = uVar9 - 0x1 & 0x3fff;
                  if (uVar11 == uVar9) {
                    if (*puVar14 != 0x0) {
                      // WARNING: Subroutine does not
                      // return
                      ASSERT_FAIL();
                    }
                    goto LAB_807eb594;
                  }
                  res = puVar18[uVar9];
                  puVar15 = puVar18 + uVar9;
                  if ((res & 0x8000) == 0x0) {
                    // WARNING: Subroutine does not return
                    ASSERT_FAIL();
                  }
                } while ((res & 0x4000) == 0x0);
                hh = DAT_80ad1160[uVar9];
                if (hh == 0xffff) {
                  hh = hash1((u32)file->backBuffer[res + 0x1 & 0xfff] << 0x8 |
                             (u32)file->backBuffer[res + 0x2 & 0xfff] << 0x10 |
                             (u32)file->backBuffer[res & 0xfff]);
                  hh = hh & 0xffff;
                  DAT_80ad1160[uVar9] = (short)hh;
                }
                if (0x3fff < hh) {
                  // WARNING: Subroutine does not return
                  ASSERT_FAIL();
                }
              } while ((uVar9 - hh & 0x3fff) < (uVar11 - hh & 0x3fff));
              *puVar14 = *puVar15;
              DAT_80ad1160[uVar11] = DAT_80ad1160[uVar9];
              *puVar15 = 0x8000;
              DAT_80ad1160[uVar9] = 0xffff;
              puVar14 = puVar15;
              uVar11 = uVar9;
              if ((*puVar15 & 0x4000) != 0x0) {
                // WARNING: Subroutine does not return
                ASSERT_FAIL();
              }
            } while (true);
          }
        LAB_807eb594:
          bVar1 = uVar2 != 0x3fff;
          puVar13 = puVar13 + 0x1;
          uVar2 = uVar2 + 0x1;
        } while (bVar1);
        uVar17 = file->iteration;
        file->field7_0x1018 = 0x0;
      }
    }
    if (0x9 < uVar17) {
      uVar9 = file->field7_0x1018;
      uVar2 = file->backBufferLocation - 0x3 & 0xfff;
      uVar11 = (u32)file->backBuffer[uVar2 + 0x1 & 0xfff] << 0x8 |
               (u32)file->backBuffer[uVar2 + 0x2 & 0xfff] << 0x10 |
               (u32)file->backBuffer[uVar2];
      iVar7 = 0x4;
      uVar17 = uVar2;
      do {
        if (uVar11 == ((u32)file->backBuffer[uVar17 - 0x3 & 0xfff] << 0x8 |
                       (u32)file->backBuffer[uVar17 - 0x2 & 0xfff] << 0x10 |
                       (u32)file->backBuffer[uVar17 - 0x4 & 0xfff]))
          goto LAB_807eb78c;
        uVar17 = uVar17 + 0x1;
        iVar7 = iVar7 + -0x1;
      } while (iVar7 != 0x0);
      uVar17 = hash1(uVar11);
      if (0x3fff < uVar17) {
        // WARNING: Subroutine does not return
        ASSERT_FAIL();
      }
      res = file->hashMap[uVar17];
      if ((res & 0x4000) != 0x0) {
        iVar7 = 0x4000;
        while (true) {
          uVar17 = uVar17 + 0x1 & 0x3fff;
          res = file->hashMap[uVar17];
          if ((res & 0x4000) == 0x0)
            break;
          iVar7 = iVar7 + -0x1;
          if (iVar7 == 0x0) {
            // WARNING: Subroutine does not return
            ASSERT_FAIL();
          }
        }
      }
      file->hashMap[uVar17] = (u16)uVar2 | 0xc000;
      uVar9 = uVar9 - (res >> 0xf);
    LAB_807eb78c:
      file->field7_0x1018 = uVar9;
    }
    iVar7 = file->copyLocation;
    if (iVar7 != -0x1) {
      if ((file->field5_0x1010 >> 0x8 & 0xff) == (u32)file->backBuffer[iVar7]) {
        uVar2 = file->backBufferLocation;
        file->backBuffer[uVar2] = (u8)(file->field5_0x1010 >> 0x8);
        uVar2 = uVar2 + 0x1 & 0xfff;
        file->backBufferLocation = uVar2;
        file->byteShifterRemaining = file->byteShifterRemaining - 0x1;
        uVar17 = iVar7 + 0x1U & 0xfff;
        file->copyLocation = uVar17;
        file->copyLength = file->copyLength + 0x1;
        if (file->copyDistance != ((uVar2 - 0x1) - uVar17 & 0xfff)) {
          // WARNING: Subroutine does not return
          ASSERT_FAIL();
        }
        return value;
      }
      goto LAB_807eaf78;
    }
  }
  uVar2 = file->copyLength;
LAB_807eb1a8:
  while (uVar2 != 0x0) {
    while (true) {
      if (file->copyLocation == -0x1) {
        // WARNING: Subroutine does not return
        ASSERT_FAIL();
      }
      if (file->codeBufferIndex == '\0') {
        file->codeBuffer[0x0] = '\0';
        uVar17 = 0x7;
        bVar5 = 0x80;
        uVar6 = '\x02';
        uVar11 = 0x1;
      } else {
        uVar11 = (u32)file->codeBufferLocation;
        uVar17 = (u32)(u8)(file->codeBufferIndex - 0x1);
        uVar6 = file->codeBufferLocation + 0x1;
        bVar5 = (u8)(0x1 << uVar17);
      }
      uVar9 = file->backBufferLocation - uVar2;
      uVar2 = uVar2 - 0x1;
      file->codeBuffer[uVar11] = file->backBuffer[uVar9 & 0xfff];
      file->codeBufferLocation = uVar6;
      file->codeBufferIndex = (u8)uVar17;
      file->copyLength = uVar2;
      file->codeBuffer[0x0] = bVar5 | file->codeBuffer[0x0];
      if (uVar17 != 0x0)
        break;
      iVar7 = Yaz_buffer_putcode_r(reent, (u8*)file);
      if (iVar7 != 0x0) {
        return -0x1;
      }
      uVar2 = file->copyLength;
      if (uVar2 == 0x0)
        goto LAB_807eb200;
    }
  }
LAB_807eb200:
  if (value == -0x1) {
    do {
      uVar17 = file->byteShifterRemaining;
      uVar2 = (u32)file->codeBufferIndex;
      iVar7 = uVar17 + 0x1;
      bVar1 = uVar2 == 0x0;
      iVar10 = uVar17 * -0x8 + 0x20;
      do {
        iVar7 = iVar7 + -0x1;
        if (iVar7 == 0x0) {
          if ((!bVar1) &&
              (iVar7 = Yaz_buffer_putcode_r(reent, (u8*)file), iVar7 != 0x0)) {
            return -0x1;
          }
          if (file->field12_0x9028 == 0x0) {
            return 0x0;
          }
          uVar2 =
              file->file_write(file->field13_0x902c, 0x1, file->field12_0x9028);
          if (uVar2 == file->field12_0x9028) {
            return 0x0;
          }
          return -0x1;
        }
        file->backBuffer[file->backBufferLocation] =
            (u8)(file->field5_0x1010 >> iVar10);
        file->backBufferLocation = file->backBufferLocation + 0x1 & 0xfff;
        if (bVar1) {
          file->codeBuffer[0x0] = (u8)uVar2;
          bVar5 = 0x80;
          uVar2 = 0x7;
          uVar6 = '\x02';
          uVar11 = 0x1;
        } else {
          uVar11 = (u32)file->codeBufferLocation;
          uVar2 = uVar2 - 0x1 & 0xff;
          uVar6 = file->codeBufferLocation + 0x1;
          bVar5 = (u8)(0x1 << uVar2);
        }
        bVar1 = uVar2 == 0x0;
        uVar17 = uVar17 - 0x1;
        iVar10 = iVar10 + 0x8;
        file->codeBuffer[uVar11] =
            (u8)(file->field5_0x1010 >>
                 file->byteShifterRemaining * (-0x8 + 0x20));
        file->codeBufferLocation = uVar6;
        file->codeBufferIndex = (u8)uVar2;
        file->codeBuffer[0x0] = bVar5 | file->codeBuffer[0x0];
        file->byteShifterRemaining = uVar17;
      } while (!bVar1);
      iVar7 = Yaz_buffer_putcode_r(reent, (u8*)file);
    } while (iVar7 == 0x0);
  } else {
    if (file->byteShifterRemaining != 0x3) {
      // WARNING: Subroutine does not return
      ASSERT_FAIL();
    }
    uVar17 = file->field5_0x1010;
    uVar2 = file->backBufferLocation;
    uVar6 = (u8)(uVar17 >> 0x8);
    file->backBuffer[uVar2] = uVar6;
    uVar2 = uVar2 + 0x1 & 0xfff;
    file->backBufferLocation = uVar2;
    file->byteShifterRemaining = 0x2;
    if ((u32)file->iteration < 0x3) {
    LAB_807eb248:
      if (file->codeBufferIndex == '\0') {
        file->codeBuffer[0x1] = uVar6;
        file->codeBufferLocation = '\x02';
        file->codeBufferIndex = '\a';
        file->codeBuffer[0x0] = 0x80;
      } else {
        bVar5 = file->codeBufferLocation;
        bVar4 = file->codeBufferIndex - 0x1;
        file->codeBuffer[bVar5] = uVar6;
        file->codeBufferLocation = bVar5 + 0x1;
        file->codeBufferIndex = bVar4;
        file->codeBuffer[0x0] = (u8)(0x1 << (u32)bVar4) | file->codeBuffer[0x0];
        if (bVar4 == 0x0) {
          iVar7 = Yaz_buffer_putcode_r(reent, (u8*)file);
          if (iVar7 != 0x0) {
            return -0x1;
          }
          uVar2 = file->backBufferLocation;
          uVar17 = file->field5_0x1010;
        }
      }
      file->copyLength = 0x0;
      file->copyDistance = 0x0;
      uVar11 = uVar2 - 0x1 & 0xfff;
      file->copyLocation = -0x1;
      uVar17 = uVar17 >> 0x10 & 0xff;
      if (file->backBuffer[uVar11] == uVar17) {
        file->copyLocation = uVar11;
      }
      uVar2 = uVar2 - 0x2 & 0xfff;
      if (uVar17 == file->backBuffer[uVar2]) {
        file->copyLocation = uVar2;
        file->copyDistance = 0x1;
      }
    } else {
      uVar11 = hash1(uVar17 >> 0x8);
      iVar7 = 0x4001;
      if (0x3fff < uVar11) {
        // WARNING: Subroutine does not return
        ASSERT_FAIL();
      }
      uVar9 = (u32)file->hashMap[uVar11];
      if ((file->hashMap[uVar11] & 0x4000) == 0x0)
        goto LAB_807eb408;
      while (true) {
        if ((uVar9 & 0x8000) == 0x0) {
          // WARNING: Subroutine does not return
          ASSERT_FAIL();
        }
        hh = uVar9 & 0xfff;
        if (uVar17 >> 0x8 ==
            ((u32)file->backBuffer[uVar9 + 0x1 & 0xfff] << 0x8 |
             (u32)file->backBuffer[uVar9 + 0x2 & 0xfff] << 0x10 |
             (u32)file->backBuffer[hh]))
          break;
        while (true) {
          iVar7 = iVar7 + -0x1;
          if (iVar7 == 0x0) {
            // WARNING: Subroutine does not return
            ASSERT_FAIL();
          }
          uVar11 = uVar11 + 0x1 & 0x3fff;
          uVar9 = (u32)file->hashMap[uVar11];
          if ((file->hashMap[uVar11] & 0x4000) != 0x0)
            break;
        LAB_807eb408:
          if ((uVar9 & 0x8000) == 0x0)
            goto LAB_807eb248;
        }
      }
      uVar17 = uVar2 - 0x1 & 0xfff;
      if (file->backBuffer[uVar17] != file->backBuffer[hh]) {
        // WARNING: Subroutine does not return
        ASSERT_FAIL();
      }
      if (uVar17 == hh) {
        // WARNING: Subroutine does not return
        ASSERT_FAIL();
      }
      uVar17 = hh + 0x1 & 0xfff;
      file->copyLength = 0x1;
      file->copyLocation = uVar17;
      file->copyDistance = (uVar2 - 0x1) - uVar17 & 0xfff;
    }
  }
  return value;
}

bool Yaz_open(Yaz_file_struct* file, WRITE_FN fptr) {
  memset(DAT_80ad1160, 0xff, sizeof(DAT_80ad1160));
  memset(file, 0, sizeof(*file));

  file->file_write = fptr;
  if (!file->file_write)
    return false;

  const u8 szsHeader[] = {
      0x59, 0x61, 0x7A, 0x30, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  int ret = file->file_write(szsHeader, sizeof(szsHeader), 1);
  assert(ret == 1);

  file->copyLocation = -1;

  return true;
}
