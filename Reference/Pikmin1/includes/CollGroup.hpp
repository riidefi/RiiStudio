#pragma once

#include "essential_functions.hpp"
#include <LibRiiEditor/common.hpp>
#include <LibCube/Common/BoundBox.hpp>

namespace libcube { namespace pikmin1 {

// Unsure of how to go abouts doing this
struct CollGroupVariables
{
	CollGroupVariables() = default;
	~CollGroupVariables() = default;

	u16 m_unk1Count = 0;
	std::vector<u8> m_vecUnk1;

	std::vector<u32> m_vecUnk2;
	u16 m_collTriCount = 0;
};

struct CollGroup
{
	constexpr static const char name[] = "Collision Group";

	AABB m_collBounds;
	CollGroupVariables m_vars;

	f32 m_gridSizeRadius;
	u32 m_gridSizeX;
	u32 m_gridSizeY;
	u32 m_blockSize;

	CollGroup() = default;
	~CollGroup() = default;

	static void onRead(oishii::BinaryReader& bReader, CollGroup& context)
	{
		skipPadding(bReader);

		// Collision boundaries!
		context.m_collBounds << bReader;

		// Figured out from decompile
		context.m_gridSizeRadius = bReader.read<f32>();
		context.m_gridSizeX = bReader.read<u32>();
		context.m_gridSizeY = bReader.read<u32>();
		
		context.m_blockSize = bReader.read<u32>();
		u32 maxCollTris = 0;
		for (u32 i = 0; i < context.m_blockSize; i++)
		{
			context.m_vars.m_unk1Count = bReader.readUnaligned<u16>();
			context.m_vars.m_collTriCount = bReader.readUnaligned<u16>();

			if (context.m_vars.m_collTriCount > maxCollTris)
				maxCollTris = context.m_vars.m_collTriCount;

			context.m_vars.m_vecUnk2.resize(context.m_vars.m_collTriCount);
			for (auto& var : context.m_vars.m_vecUnk2)
				var = bReader.readUnaligned<u32>();

			if (context.m_vars.m_unk1Count)
			{
				context.m_vars.m_vecUnk2.resize(context.m_vars.m_unk1Count);
				for (auto& byte : context.m_vars.m_vecUnk2)
					byte = bReader.readUnaligned<u8>();
			}
		}
		DebugReport("Max collision triangles within a block: %u\n", maxCollTris);

		// This loops only actual purpose is to read ints and to
		// figure out the maximum distance from the centre of the map
		for (u32 i = 0; i < context.m_gridSizeY; i++)
		{
			for (u32 j = 0; j < context.m_gridSizeX; j++)
			{
				// const u32 unk1 =
					bReader.readUnaligned<u32>();
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
