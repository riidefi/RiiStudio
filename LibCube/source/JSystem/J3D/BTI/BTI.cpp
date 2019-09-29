#include "BTI.hpp"

namespace libcube { namespace pikmin1 {

void BTI::onRead(oishii::BinaryReader& bReader, BTI& context)
{
	context.importBTI(bReader);
}

void BTI::importBTI(oishii::BinaryReader& bReader)
{
	const u32 bti_offset = bReader.tell();

	m_format = static_cast<BTIFormats>(bReader.read<u8>());
	m_transparent = bReader.read<u8>();

	m_width = bReader.read<u16>();
	m_height = bReader.read<u16>();

	m_wrapS = bReader.read<u8>();
	m_wrapT = bReader.read<u8>();

	m_fPalette = bReader.read<u8>();
	m_paletteFormat = bReader.read<u8>();
	m_nPalette = bReader.read<u16>();
	m_toPalette = bReader.read<u32>();

	m_mipmap = bReader.read<u8>();
	m_edgeLOD = bReader.read<u8>();
	m_biasClamp = bReader.read<u8>();
	m_maxAniso = bReader.read<u8>();
	m_minFilter = bReader.read<u8>();
	m_magFilter = bReader.read<u8>();

	m_minLOD = bReader.read<s8>();
	m_maxLOD = bReader.read<s8>();

	m_nImage = bReader.read<u8>();
	m_lodBias = bReader.read<s16>();
	m_toImage = bReader.read<u32>();

	// seek to the image data
	bReader.seekSet(bti_offset);
	bReader.seek(m_toImage);

	const u32 imagesize = (m_toPalette == 0)
		? bReader.endpos() - m_toImage
		: m_toPalette - m_toImage;

	m_imageData.resize(imagesize);
	for (auto& byte : m_imageData)
		byte = bReader.read<u8>();

	// sometimes there are no palettes
	if (m_toPalette != 0)
	{
		// seek to the palette data
		bReader.seekSet(bti_offset);
		bReader.seek(m_toPalette);

		const u32 palettesize = m_nPalette * 2;
		m_paletteData.resize(palettesize);
		for (auto& byte : m_paletteData)
			byte = bReader.read<u8>();
	}
}

}

}
