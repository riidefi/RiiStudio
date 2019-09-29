#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct BaseRoomInfo
{
	constexpr static const char name[] = "Base Room Information";
	u32 m_unk1;

	BaseRoomInfo() = default;
	~BaseRoomInfo() = default;

	static void onRead(oishii::BinaryReader& bReader, BaseRoomInfo& context)
	{
		context.m_unk1 = bReader.read<u32>();
	}
};

inline void operator<<(BaseRoomInfo& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<BaseRoomInfo, oishii::Direct, false>(context);
}

}

}
