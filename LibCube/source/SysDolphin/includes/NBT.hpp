#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct NBT
{
	constexpr static const char name[] = "NBT";

	glm::vec3 m_normals; //N
	glm::vec3 m_binormals; //B
	glm::vec3 m_tangents; //T

	NBT() = default;
	~NBT() = default;

	static void onRead(oishii::BinaryReader& bReader, NBT& context)
	{
		read(bReader, context.m_normals);
		read(bReader, context.m_binormals);
		read(bReader, context.m_tangents);
	}
};

inline void operator<<(NBT& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<NBT, oishii::Direct, false>(context);
}

}

}
