#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct VtxMatrix
{
	constexpr static const char name[] = "VtxMatrix";

	bool m_partiallyWeighted;
	u16 m_index;

	VtxMatrix() = default;
	~VtxMatrix() = default;

	static void onRead(oishii::BinaryReader& bReader, VtxMatrix& context)
	{
		const s16 idx = bReader.read<s16>();
		context.m_index = (context.m_partiallyWeighted = idx >= 0) ? idx : -idx - 1;
	}
};

inline void operator<<(VtxMatrix& context, oishii::BinaryReader& reader)
{
	reader.dispatch<VtxMatrix, oishii::Direct, false>(context);
}

} }
