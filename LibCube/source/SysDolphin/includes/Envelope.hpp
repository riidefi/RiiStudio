#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct Envelope
{
	constexpr static const char name[] = "Envelope";

	std::vector<u16> m_indexes;
	std::vector<f32> m_weights;

	static void onRead(oishii::BinaryReader& bReader, Envelope& context)
	{
		context.m_indexes.resize(bReader.read<u16>());
		context.m_weights.resize(context.m_indexes.size());

		for (u32 i = 0; i < context.m_indexes.size(); i++)
		{
			context.m_indexes[i] = bReader.read<u16>();
			context.m_weights[i] = bReader.read<f32>();
		}
	}
};

inline void read(oishii::BinaryReader& reader, Envelope& evp)
{
	reader.dispatch<Envelope, oishii::Direct, false>(evp);
}

}

}
