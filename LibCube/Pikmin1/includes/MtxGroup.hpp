#pragma once

#include "essential_functions.hpp"
#include "DispList.hpp"

namespace libcube { namespace pikmin1 {

struct MtxGroup
{
	constexpr static const char name[] = "Matrix Group";

	std::vector<u16> m_dependant;
	std::vector<DispList> m_dispLists;

	MtxGroup() = default;
	~MtxGroup() = default;

	static void onRead(oishii::BinaryReader& bReader, MtxGroup& context)
	{
		// Unknown purpose of the array, store it anyways
		context.m_dependant.resize(bReader.read<u32>());
		for (auto& isDependant : context.m_dependant)
			isDependant = bReader.read<u16>();

		context.m_dispLists.resize(bReader.read<u32>());
		for (auto& dLists : context.m_dispLists)
		{
			// Read DispLists
			dLists << bReader;
		}
	}
};
inline void operator<<(MtxGroup& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<MtxGroup, oishii::Direct, false>(context);
}

}

}
