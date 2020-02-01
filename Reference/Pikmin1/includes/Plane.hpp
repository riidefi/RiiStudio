#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct Plane
{
	constexpr static const char name[] = "Plane";
/// The direction this plane is facing at.
/** This direction vector is always normalized. If you assign to this directly, please remember to only
	assign normalized direction vectors. */
	glm::vec3 m_normal;
/// The offset of this plane from the origin. [similarOverload: normal]
/** The value -d gives the signed distance of this plane from origin.
	Denoting normal:=(a,b,c), this class uses the convention ax+by+cz = d, which means that:
	 - If this variable is positive, the vector space origin (0,0,0) is on the negative side of this plane.
	 - If this variable is negative, the vector space origin (0,0,0) is on the on the positive side of this plane.
	@note Some sources use the opposite convention ax+by+cz+d = 0 to define the variable d. When comparing equations
		and algorithm regarding planes, always make sure you know which convention is being used, since it affects the
		sign of d. */
	f32 m_distance;

	Plane() = default;
	~Plane() = default;

	static void onRead(oishii::BinaryReader& bReader, Plane& context)
	{
		read(bReader, context.m_normal);
		context.m_distance = bReader.read<f32>();
	}
};
inline void operator<<(Plane& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<Plane, oishii::Direct, false>(context);
}

}

}
