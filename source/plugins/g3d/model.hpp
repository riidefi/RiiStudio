#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

#include "bone.hpp"
#include "material.hpp"

namespace riistudio::g3d {


enum class ScalingRule
{
	Standard,
	XSI,
	Maya
};
enum class TextureMatrixMode
{
	Maya,
	XSI,
	Max
};
enum class EnvelopeMatrixMode
{
	Normal,
	Approximation,
	Precision
};

struct G3DModel {
	virtual ~G3DModel() = default;
	// Shallow comparison
	bool operator==(const G3DModel& rhs) const { return true;}
	const G3DModel& operator=(const G3DModel& rhs) {
		return *this;
	}

	ScalingRule mScalingRule;
	TextureMatrixMode mTexMtxMode;
	EnvelopeMatrixMode mEvpMtxMode;
	std::string sourceLocation;
	lib3d::AABB aabb;
};

struct G3DModelAccessor : public kpi::NodeAccessor<G3DModel> {
	KPI_NODE_FOLDER_SIMPLE(Material);
	KPI_NODE_FOLDER_SIMPLE(Bone);
};

} // namespace riistudio::g3d
