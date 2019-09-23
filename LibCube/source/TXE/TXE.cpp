#include "TXE.hpp"

namespace libcube {

namespace pikmin1 {

void TXE::read(oishii::BinaryReader& bReader)
{
	m_width = bReader.read<u16>();
	m_height = bReader.read<u16>();
	m_format = (TXEFormats)bReader.read<u16>();
	bReader.read<u16>(); // unused variable
	m_dataSize = bReader.read<u32>();
	m_imageData.resize(m_dataSize);

	// Unsure as to why they do this, but its in the source code so lollage
	for (u32 i = 0; i < 5; i++)
		bReader.read<u32>();

	for (auto& byte : m_imageData)
		byte = bReader.read<u8>();
}

void TXE::readModFile(oishii::BinaryReader & bReader)
{
	m_width = bReader.read<u16>();
	m_height = bReader.read<u16>();
	m_format = (TXEFormats)bReader.read<u32>();
	bReader.read<u32>();

	for (u32 i = 0; i < 4; i++)
		bReader.read<u32>();

	m_dataSize = bReader.read<u32>();
	m_imageData.resize(m_dataSize);
	for (auto& byte : m_imageData)
		byte = bReader.read<u8>();
}

}

}
