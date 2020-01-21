#pragma once

#include "essential_functions.hpp"
#include "MtxGroup.hpp"

namespace libcube { namespace pikmin1 {

struct Batch
{
	constexpr static const char name[] = "Batch";

	u32 m_jointIndex = 0;
	u32 m_depMTXGroups = 0;
	int m_vcd;

	std::vector<MtxGroup> m_mtxGroups;

	Batch() = default;
	~Batch() = default;

	static void onRead(oishii::BinaryReader& bReader, Batch& context)
	{
		// Read the batch variables
		context.m_jointIndex = bReader.read<u32>();
		context.m_vcd = bReader.read<u32>();

		context.m_mtxGroups.resize(bReader.read<u32>());
		context.m_depMTXGroups = 0;
		for (auto& mGroup : context.m_mtxGroups)
		{
			// Read mtxgroup
			mGroup << bReader;
			// Assign appropriate mtx dependant faces (?)
			if (mGroup.m_dependant.size() > context.m_depMTXGroups)
				context.m_depMTXGroups = static_cast<u32>(mGroup.m_dependant.size());
		}
	}
};

inline void operator<<(Batch& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<Batch, oishii::Direct, false>(context);
}

} }
