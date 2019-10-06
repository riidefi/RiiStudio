#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

// Used to store minimum and maximum boundary values, denoted by 2 vec3s
struct BoundBox
{
	constexpr static const char name[] = "Bounding Box";

	glm::vec3 m_minBounds;
	glm::vec3 m_maxBounds;

	BoundBox() = default;
	~BoundBox() = default;

	static void onRead(oishii::BinaryReader& bReader, BoundBox& context)
	{
		read(bReader, context.m_minBounds);
		read(bReader, context.m_maxBounds);
	}

	void expandBound(BoundBox& expandBy)
	{
		if (expandBy.m_minBounds.x < m_minBounds.x)
			m_minBounds.x = expandBy.m_minBounds.x;
		if (expandBy.m_minBounds.y < m_minBounds.y)
			m_minBounds.y = expandBy.m_minBounds.y;
		if (expandBy.m_minBounds.z < m_minBounds.z)
			m_minBounds.z = expandBy.m_minBounds.z;
		if (expandBy.m_maxBounds.x > m_maxBounds.x)
			m_maxBounds.x = expandBy.m_maxBounds.x;
		if (expandBy.m_maxBounds.y > m_maxBounds.y)
			m_maxBounds.y = expandBy.m_maxBounds.y;
		if (expandBy.m_maxBounds.z > m_maxBounds.z)
			m_maxBounds.z = expandBy.m_maxBounds.z;
	}
};

inline void operator<<(BoundBox& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<BoundBox, oishii::Direct, false>(context);
}

}

}
