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
	BTIFormats m_format = BTIFormats::I4; // GXTexFmt
	u8 m_transparent = 0;

	u16 m_width = 0;
	u16 m_height = 0;

	u8 m_wrapS = 0; // GXTexWrapMode
	u8 m_wrapT = 0; // GXTexWrapMode

	u8 m_fPalette = 0;
	u8 m_paletteFormat = 0; // GXTlutFormat
	u16 m_nPalette = 0;
	u32 m_toPalette = 0;

	u8 m_mipmap = 0;
	u8 m_edgeLOD = 0;
	u8 m_biasClamp = 0;
	u8 m_maxAniso = 0; // GXAnisotropy
	u8 m_minFilter = 0; // GXTexFilter
	u8 m_magFilter = 0; // GXTexFilter

	s8 m_minLOD = 0;
	s8 m_maxLOD = 0;

	u8 m_nImage = 0;
	s16 m_lodBias = 0;
	u32 m_toImage = 0;

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
