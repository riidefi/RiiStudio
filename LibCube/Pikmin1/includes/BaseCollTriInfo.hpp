#pragma once

#include "essential_functions.hpp"
#include "Plane.hpp"

namespace libcube { namespace pikmin1 {

struct BaseCollTriInfo
{
	constexpr static const char name[] = "Base Collision Triangle Information";
	u32 m_unk1;
	// These form a triangle of collision
	u32 m_faceA;
	u32 m_faceB;
	u32 m_faceC;

	u16 m_unk5;
	u16 m_unk6;
	u16 m_unk7;
	u16 m_unk8;

	Plane m_collisionPlane;

	BaseCollTriInfo() = default;
	~BaseCollTriInfo() = default;

	// WHAT EVEN IS THIS STRUCT SUPPOSED TO BE?

	static void onRead(oishii::BinaryReader& bReader, BaseCollTriInfo& context)
	{
		context.m_unk1 = bReader.read<u32>();
		context.m_faceA = bReader.read<u32>();
		context.m_faceB = bReader.read<u32>();
		context.m_faceC = bReader.read<u32>();

		context.m_unk5 = bReader.read<u16>();
		context.m_unk6 = bReader.read<u16>();
		context.m_unk7 = bReader.read<u16>();
		context.m_unk8 = bReader.read<u16>();

		context.m_collisionPlane << bReader;
	}
};

inline void operator<<(BaseCollTriInfo& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<BaseCollTriInfo, oishii::Direct, false>(context);
}

}

}
