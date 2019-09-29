#pragma once

#include "essential_functions.hpp"
#include "BoundBox.hpp"

namespace libcube { namespace pikmin1 {

struct CollGroup
{
	constexpr static const char name[] = "Collision Group";

	BoundBox m_collBounds;

	float m_unk1;
	u32 m_unkCount1;
	u32 m_unkCount2;
	u32 m_blockSize;

	CollGroup() = default;
	~CollGroup() = default;

	static void onRead(oishii::BinaryReader& bReader, CollGroup& context)
	{
		skipPadding(bReader);

		context.m_collBounds << bReader;
		context.m_unk1 = bReader.read<f32>();
		context.m_unkCount1 = bReader.read<u32>();
		context.m_unkCount2 = bReader.read<u32>();
		context.m_blockSize = bReader.read<u32>();

		u32 maxCollTris = 0;
		for (u32 i = 0; i < context.m_blockSize; i++)
		{
			const u16 unkCount1 = bReader.read<u16>();
			const u16 collTriCount = bReader.read<u16>();

			if (collTriCount > maxCollTris)
				maxCollTris = collTriCount;

			for (u32 j = 0; j < collTriCount; j++)
				bReader.read<u32>();

			if (unkCount1)
				for (u32 k = 0; k < unkCount1; k++)
					bReader.read<u8>();
		}

		DebugReport("Max collision triangles within a block: %u\n", maxCollTris);

		for (u32 i = 0; i < context.m_unkCount2; i++)
		{
			for (u32 j = 0; j < context.m_unkCount1; j++)
			{
				const u32 unk1 = bReader.read<u32>();
			}
		}

		skipPadding(bReader);
	}
};

inline void operator<<(CollGroup& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<CollGroup, oishii::Direct, false>(context);
}

}

}
