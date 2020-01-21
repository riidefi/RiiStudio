#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

// Unused in game

struct BaseRoomInfo
{
	constexpr static const char name[] = "Base Room Information";
	u32 m_size;

	BaseRoomInfo() = default;
	~BaseRoomInfo() = default;

	static void onRead(oishii::BinaryReader& bReader, BaseRoomInfo& context)
	{
		context.m_size = bReader.readUnaligned<u32>();
	}
};

inline void operator<<(BaseRoomInfo& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<BaseRoomInfo, oishii::Direct, false>(context);
}

}

}
