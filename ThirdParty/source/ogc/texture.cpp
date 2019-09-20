#include "texture.h"

enum {
	_CTF = 0x20,
	_ZTF = 0x10,
};
enum class TextureFormat
{
	I4 = 0x0,
	I8 = 0x1,
	IA4 = 0x2,
	IA8 = 0x3,
	RGB565 = 0x4,
	RGB5A3 = 0x5,
	RGBA8 = 0x6,
	CMPR = 0xE,
};

enum class CopyTextureFormat
{
	R4 = 0x0 | _CTF,
	RA4 = 0x2 | _CTF,
	RA8 = 0x3 | _CTF,
	YUVA8 = 0x6 | _CTF,
	A8 = 0x7 | _CTF,
	R8 = 0x8 | _CTF,
	G8 = 0x9 | _CTF,
	B8 = 0xA | _CTF,
	RG8 = 0xB | _CTF,
	GB8 = 0xC | _CTF,
};
enum class ZTextureFormat
{
	Z8 = 0x1 | _ZTF,
	Z16 = 0x3 | _ZTF,
	Z24X8 = 0x6 | _ZTF,

	Z4 = 0x0 | _ZTF | _CTF,
	Z8M = 0x9 | _ZTF | _CTF,
	Z8L = 0xA | _ZTF | _CTF,
	Z16L = 0xC | _ZTF | _CTF,

};
u32 GetTexBufferSize(u16 wd, u16 ht, u32 fmt, u8 mipmap, u8 maxlod)
{
	u32 xshift, yshift, xtiles, ytiles, bitsize, size;

	switch (fmt) {
	case (u32)TextureFormat::I4:
	case (u32)TextureFormat::CMPR:
	case (u32)CopyTextureFormat::R4:
	case (u32)CopyTextureFormat::RA4:
	case (u32)ZTextureFormat::Z4:
		xshift = 3;
		yshift = 3;
		break;
	case (u32)ZTextureFormat::Z8:
	case (u32)TextureFormat::I8:
	case (u32)TextureFormat::IA4:
	case (u32)CopyTextureFormat::A8:
	case (u32)CopyTextureFormat::R8:
	case (u32)CopyTextureFormat::G8:
	case (u32)CopyTextureFormat::B8:
	case (u32)CopyTextureFormat::RG8:
	case (u32)CopyTextureFormat::GB8:
	case (u32)ZTextureFormat::Z8M:
	case (u32)ZTextureFormat::Z8L:
		xshift = 3;
		yshift = 2;
		break;
	case (u32)TextureFormat::IA8:
	case (u32)ZTextureFormat::Z16:
	case (u32)ZTextureFormat::Z24X8:
	case (u32)TextureFormat::RGB565:
	case (u32)TextureFormat::RGB5A3:
	case (u32)TextureFormat::RGBA8:
	case (u32)ZTextureFormat::Z16L:
	case (u32)CopyTextureFormat::RA8:
		xshift = 2;
		yshift = 2;
		break;
	default:
		xshift = 2;
		yshift = 2;
		break;
	}

	bitsize = 32;
	if (fmt == (u32)TextureFormat::RGBA8 || fmt == (u32)ZTextureFormat::Z24X8) bitsize = 64;

	size = 0;
	if (mipmap) {
		u32 cnt = (maxlod & 0xff);
		while (cnt) {
			u32 w = wd & 0xffff;
			u32 h = ht & 0xffff;
			xtiles = ((w + (1 << xshift)) - 1) >> xshift;
			ytiles = ((h + (1 << yshift)) - 1) >> yshift;
			if (cnt == 0) return size;

			size += ((xtiles*ytiles)*bitsize);
			if (w == 0x0001 && h == 0x0001) return size;
			if (wd > 0x0001) wd = (w >> 1);
			else wd = 0x0001;
			if (ht > 0x0001) ht = (h >> 1);
			else ht = 0x0001;

			--cnt;
		}
		return size;
	}

	wd &= 0xffff;
	xtiles = (wd + ((1 << xshift) - 1)) >> xshift;

	ht &= 0xffff;
	ytiles = (ht + ((1 << yshift) - 1)) >> yshift;

	size = ((xtiles*ytiles)*bitsize);

	return size;
}
