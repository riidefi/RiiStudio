#pragma once

#include "essential_functions.hpp"
#include "Colour.hpp"

namespace libcube { namespace pikmin1 {

struct Material
{
	u32 m_hasPE; // pixel engine
	u32 m_unk2;
	Colour m_unkColour1;

	// if (hasPE & 1)
	u32 m_unk3;

	void read(oishii::BinaryReader& bReader)
	{
		m_hasPE = bReader.read<u32>();
		m_unk2 = bReader.read<u32>();
		readChunk(bReader, m_unkColour1);

		if (m_hasPE & 1)
		{
			m_unk3 = bReader.read<u32>();
			// PVWPolygonColourInfo
			// PVWLightingInfo
			// PVWPeInfo
			// PVWTextureInfo
		}
	}
};

}

}
