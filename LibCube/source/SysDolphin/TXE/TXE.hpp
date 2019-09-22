#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <SysDolphin/includes/essential_functions.hpp>

#include <vector>

namespace libcube { namespace pikmin1 {

enum class TXEFormats : u16
{
	Rgb565 = 0,
	Cmpr, // 1
	Rgb5a3, // 2
	I4, // 3
	I8, // 4
	Ia4, // 5
	Ia8, // 6
	Rgba8 //7
};

struct TXE
{
	u16 m_width = 0;
	u16 m_height = 0;
	u16 m_unk1 = 0;
	u16 m_formatShort = 0;
	TXEFormats m_format;
	u32 m_unk2 = 0;

	std::vector<u8> m_imageData;

	TXE() = default;
	~TXE() = default;

	constexpr static const char name[] = "Pikmin 1 Texture File";

	static void onRead(oishii::BinaryReader&, TXE&);

	void importTXE(oishii::BinaryReader&);
	void importMODTXE(oishii::BinaryReader&);
};
inline void read(oishii::BinaryReader& reader, TXE& clr)
{
	reader.dispatch<TXE, oishii::Direct, false>(clr);
}

}

}
