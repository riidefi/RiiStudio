#pragma once

#include "essential_functions.hpp"
#include "MtxGroup.hpp"

namespace libcube { namespace pikmin1 {

//TODO: VtxDescriptor
struct VtxDescriptor
{
	u32 m_originalVCD;

	VtxDescriptor() = default;
	~VtxDescriptor() = default;

	void toPikmin1(u32 vcd)
	{
		m_originalVCD = vcd;
	}
};

struct Batch
{
	constexpr static const char name[] = "Batch";

	u32 m_embossBump = 0;
	u32 m_depMTXGroups = 0;
	VtxDescriptor m_vcd;

	std::vector<MtxGroup> m_mtxGroups;

	Batch() = default;
	~Batch() = default;

	static void onRead(oishii::BinaryReader& bReader, Batch& context)
	{
		// Read the batch variables
		context.m_embossBump = bReader.read<u32>(); // whether mesh is using emboss NBT or not
		context.m_vcd.toPikmin1(bReader.read<u32>());

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
