#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <vector>

namespace libcube { namespace pikmin1 {

enum class BTIFormats : u8
{
	I4 = 0x00,
	I8 = 0x01,
	Ia4 = 0x02,
	Ia8 = 0x03,
	Rgb565 = 0x04,
	Rgb5a3 = 0x05,
	Rgba32 = 0x06,
	C4 = 0x08,
	C8 = 0x09,
	C14x2 = 0x0A,
	Cmpr = 0x0E
};

struct BTI
{
	BTIFormats m_format; // GXTexFmt
	u8 m_transparent;

	u16 m_width;
	u16 m_height;

	u8 m_wrapS; // GXTexWrapMode
	u8 m_wrapT; // GXTexWrapMode

	u8 m_fPalette;
	u8 m_paletteFormat; // GXTlutFormat
	u16 m_nPalette;
	u32 m_toPalette;

	u8 m_mipmap;
	u8 m_edgeLOD;
	u8 m_biasClamp;
	u8 m_maxAniso; // GXAnisotropy
	u8 m_minFilter; // GXTexFilter
	u8 m_magFilter; // GXTexFilter

	s8 m_minLOD;
	s8 m_maxLOD;

	u8 m_nImage;
	s16 m_lodBias;
	u32 m_toImage;

	std::vector<u8> m_imageData;
	std::vector<u8> m_paletteData;

	BTI() = default;
	~BTI() = default;

	constexpr static const char name[] = "J3D Texture File";

	static void onRead(oishii::BinaryReader&, BTI&);

	void importBTI(oishii::BinaryReader&);
};
inline void read(oishii::BinaryReader& reader, BTI& clr)
{
	reader.dispatch<BTI, oishii::Direct, false>(clr);
}

}

}
