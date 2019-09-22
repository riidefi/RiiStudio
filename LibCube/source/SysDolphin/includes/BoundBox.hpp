#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct BoundBox
{
	constexpr static const char name[] = "Bounding Box";

	glm::vec3 m_minBounds;
	glm::vec3 m_maxBounds;

	static void onRead(oishii::BinaryReader& bReader, BoundBox& context)
	{
		read(bReader, context.m_minBounds);
		read(bReader, context.m_maxBounds);
	}
};

}

}
