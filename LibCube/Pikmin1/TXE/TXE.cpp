#include "TXE.hpp"
#include <LibRiiEditor/common.hpp>

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
	m_txeImageData.resize(bReader.read<u32>());

	skipPadding(bReader);

	for (auto& pixelData : m_txeImageData)
		pixelData = bReader.read<u8>();

	decode();
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
	m_txeImageData.resize(bReader.read<u32>());

	for (auto& pixelData : m_txeImageData)
		pixelData = bReader.read<u8>();
}

inline DecodingTextureFormat TXE::getDTF() const
{
	switch (m_format)
	{
	case TXEFormats::Rgb565:
		return DecodingTextureFormat::Rgb565;
	case TXEFormats::Cmpr:
		return DecodingTextureFormat::Cmpr;
	case TXEFormats::Rgb5a3:
		return DecodingTextureFormat::Rgb5a3;
	case TXEFormats::I4:
		return DecodingTextureFormat::I4;
	case TXEFormats::I8:
		return DecodingTextureFormat::I8;
	case TXEFormats::Ia4:
		return DecodingTextureFormat::Ia4;
	case TXEFormats::Ia8:
		return DecodingTextureFormat::Ia8;
	case TXEFormats::Rgba8:
		return DecodingTextureFormat::Rgba8;
	default:
		DebugReport("TXE Texture format given is not able to be converted! Format %u", m_formatShort);
		return DecodingTextureFormat::I4;
	}
}

void TXE::decode()
{
	DecodingTextureFormat fmt = getDTF();

	m_convImageData = TextureDecoding::decodeVec(m_txeImageData.data(),
		m_width, m_height, static_cast<u32>(fmt),
		0, nullptr, 0);


}

}

}
