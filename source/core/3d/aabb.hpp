#pragma once

#include <vendor/glm/vec3.hpp>

namespace riistudio::core {

//! Axis-aligned bounding box
//!
struct AABB
{
	inline void expandBound(const AABB& other)
	{
		if (other.m_minBounds.x < m_minBounds.x)
			m_minBounds.x = other.m_minBounds.x;
		if (other.m_minBounds.y < m_minBounds.y)
			m_minBounds.y = other.m_minBounds.y;
		if (other.m_minBounds.z < m_minBounds.z)
			m_minBounds.z = other.m_minBounds.z;
		if (other.m_maxBounds.x > m_maxBounds.x)
			m_maxBounds.x = other.m_maxBounds.x;
		if (other.m_maxBounds.y > m_maxBounds.y)
			m_maxBounds.y = other.m_maxBounds.y;
		if (other.m_maxBounds.z > m_maxBounds.z)
			m_maxBounds.z = other.m_maxBounds.z;
	}

	glm::vec3 m_minBounds;
	glm::vec3 m_maxBounds;
};

} // namespace riistudio::core
