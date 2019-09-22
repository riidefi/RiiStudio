#include "TXE.hpp"

namespace libcube { namespace pikmin1 {

void TXE::onRead(oishii::BinaryReader& bReader, TXE& context)
{
	// This works because if the TXE is in a model file, it isn't in the beginning of the file.
	if (bReader.tell() != 0)
	{
		context.importMODTXE(bReader);
	}
	else
	{
		context.importTXE(bReader);
	}
}

void TXE::importTXE(oishii::BinaryReader& bReader)
{
	// Read essential image properties
	m_width = bReader.read<u16>();
	m_height = bReader.read<u16>();
	m_formatShort = bReader.read<u16>();
	if (m_formatShort <= 7)
		m_format = static_cast<TXEFormats>(m_formatShort);

	// TODO: figure out what this variable means
	m_unk1 = bReader.read<u16>();

	// Read data size (multiple of 32)
	m_imageData.resize(bReader.read<u32>());

	skipPadding(bReader);

	for (auto& pixelData : m_imageData)
		pixelData = bReader.read<u8>();
}

void TXE::importMODTXE(oishii::BinaryReader& bReader)
{
	// Read image properties
	m_width = bReader.read<u16>();
	m_height = bReader.read<u16>();
	// TODO: figure out what this variable means
	m_unk1 = bReader.read<u16>();
	m_formatShort = bReader.read<u16>();
	if (m_formatShort <= 7)
		m_format = static_cast<TXEFormats>(m_formatShort);

	// TODO: figure out what this variable means
	m_unk2 = bReader.read<u32>();

	for (u32 i = 0; i < 4; i++)
		bReader.read<u32>(); // padding

	// Read data size (multiple of 32)
	m_imageData.resize(bReader.read<u32>());

	for (auto& pixelData : m_imageData)
		pixelData = bReader.read<u8>();
}

}

}
