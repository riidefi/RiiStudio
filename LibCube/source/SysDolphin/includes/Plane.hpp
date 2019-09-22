#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct Plane
{
	constexpr static const char name[] = "Plane";
	glm::vec3 m_unk1;
	f32 m_unk2;

	static void onRead(oishii::BinaryReader& bReader, Plane& context)
	{
		read(bReader, context.m_unk1);
		context.m_unk2 = bReader.read<f32>();
	}
};

}

}
