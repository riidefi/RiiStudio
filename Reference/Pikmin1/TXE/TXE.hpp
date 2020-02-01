#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <LibCube/Pikmin1/includes/essential_functions.hpp>
#include <ThirdParty/TextureDecoding.hpp>

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

enum class DecodingTextureFormat : u32
{
	I4 = 0x0,
	I8 = 0x1,
	Ia4 = 0x2,
	Ia8 = 0x3,
	Rgb565 = 0x4,
	Rgb5a3 = 0x5,
	Rgba8 = 0x6,
	Cmpr = 0xE,
};

struct TXE
{
	u16 m_width = 0;
	u16 m_height = 0;
	u16 m_unk1 = 0;
	u16 m_formatShort = 0;
	TXEFormats m_format = (TXEFormats)0;
	u32 m_unk2 = 0;

	std::vector<u8> m_txeImageData;
	std::vector<u8> m_convImageData;

	TXE() = default;
	~TXE() = default;

	constexpr static const char name[] = "Pikmin 1 Texture File";

	static void onRead(oishii::BinaryReader&, TXE&);

	void importTXE(oishii::BinaryReader&);
	void importMODTXE(oishii::BinaryReader&);

private:
	inline DecodingTextureFormat getDTF() const;
	void decode();
};

inline void operator<<(TXE& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<TXE, oishii::Direct, false>(context);
}

}

}
