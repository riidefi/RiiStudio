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
uint32_t GetTexBufferSize(uint16_t wd, uint16_t ht, uint32_t fmt, uint8_t mipmap, uint8_t maxlod)
{
	uint32_t xshift, yshift, xtiles, ytiles, bitsize, size;

	switch (fmt) {
	case (uint32_t)TextureFormat::I4:
	case (uint32_t)TextureFormat::CMPR:
	case (uint32_t)CopyTextureFormat::R4:
	case (uint32_t)CopyTextureFormat::RA4:
	case (uint32_t)ZTextureFormat::Z4:
		xshift = 3;
		yshift = 3;
		break;
	case (uint32_t)ZTextureFormat::Z8:
	case (uint32_t)TextureFormat::I8:
	case (uint32_t)TextureFormat::IA4:
	case (uint32_t)CopyTextureFormat::A8:
	case (uint32_t)CopyTextureFormat::R8:
	case (uint32_t)CopyTextureFormat::G8:
	case (uint32_t)CopyTextureFormat::B8:
	case (uint32_t)CopyTextureFormat::RG8:
	case (uint32_t)CopyTextureFormat::GB8:
	case (uint32_t)ZTextureFormat::Z8M:
	case (uint32_t)ZTextureFormat::Z8L:
		xshift = 3;
		yshift = 2;
		break;
	case (uint32_t)TextureFormat::IA8:
	case (uint32_t)ZTextureFormat::Z16:
	case (uint32_t)ZTextureFormat::Z24X8:
	case (uint32_t)TextureFormat::RGB565:
	case (uint32_t)TextureFormat::RGB5A3:
	case (uint32_t)TextureFormat::RGBA8:
	case (uint32_t)ZTextureFormat::Z16L:
	case (uint32_t)CopyTextureFormat::RA8:
		xshift = 2;
		yshift = 2;
		break;
	default:
		xshift = 2;
		yshift = 2;
		break;
	}

	bitsize = 32;
	if (fmt == (uint32_t)TextureFormat::RGBA8 || fmt == (uint32_t)ZTextureFormat::Z24X8) bitsize = 64;

	size = 0;
	if (mipmap) {
		uint32_t cnt = (maxlod & 0xff);
		while (cnt) {
			uint32_t w = wd & 0xffff;
			uint32_t h = ht & 0xffff;
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
