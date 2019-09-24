#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <vector>

namespace libcube {

namespace pikmin1 {

enum TXEFormats
{
	RGB565 = 0,
	CMPR, // 1
	RGB5A3, // 2
	I4, // 3
	I8, // 4
	IA4, // 5
	IA8, // 6
	RGBA8 //7
};

class TXE final
{
private:
	u16 m_width = 0;
	u16 m_height = 0;
	TXEFormats m_format;
	u32 m_dataSize = 0;
	std::vector<u8> m_imageData;


public:
	TXE() = default;
	~TXE() = default;

	constexpr static const char name[] = "TeXturE file (TXE)";

	void read(oishii::BinaryReader&);
	void readModFile(oishii::BinaryReader&);

};

}

}
