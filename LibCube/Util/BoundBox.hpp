#pragma once

#include <ThirdParty/glm/vec3.hpp>
#include <LibCube/Util/glm_serialization.hpp>

namespace libcube {

//! Axis-aligned bounding box
//!
struct AABB
{
	constexpr static const char name[] = "Bounding Box";

	glm::vec3 m_minBounds;
	glm::vec3 m_maxBounds;

	AABB() = default;
	~AABB() = default;

	AABB(const glm::vec3& m, const glm::vec3& M)
		: m_minBounds(m), m_maxBounds(M)
	{}

	static void onRead(oishii::BinaryReader& reader, AABB& context)
	{
		context.m_minBounds << reader;
		context.m_maxBounds << reader;
	}

	void expandBound(AABB& expandBy)
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

inline void operator<<(AABB& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<AABB, oishii::Direct, false>(context);
}

} // namespace libcube
