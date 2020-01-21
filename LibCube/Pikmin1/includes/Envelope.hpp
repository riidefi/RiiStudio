#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct Envelope
{
	constexpr static const char name[] = "Envelope";

	std::vector<u16> m_indices;
	std::vector<f32> m_weights;

	Envelope() = default;
	~Envelope() = default;

	static void onRead(oishii::BinaryReader& bReader, Envelope& context)
	{
		context.m_indices.resize(bReader.read<u16>());
		context.m_weights.resize(context.m_indices.size());

		for (u32 i = 0; i < context.m_indices.size(); i++)
		{
			context.m_indices[i] = bReader.readUnaligned<u16>();
			context.m_weights[i] = bReader.readUnaligned<f32>();
		}
	}
};

inline void operator<<(Envelope& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<Envelope, oishii::Direct, false>(context);
}

}

}
