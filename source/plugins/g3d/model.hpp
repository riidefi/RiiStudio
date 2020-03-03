#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

#include "bone.hpp"
#include "material.hpp"

namespace riistudio::g3d {

struct G3DModel {
	virtual ~G3DModel() = default;
	// Shallow comparison
	bool operator==(const G3DModel& rhs) const { return true;}
	const G3DModel& operator=(const G3DModel& rhs) {
		return *this;
	}
};

struct G3DModelAccessor : public kpi::NodeAccessor<G3DModel> {
	KPI_NODE_FOLDER_SIMPLE(Material);
	KPI_NODE_FOLDER_SIMPLE(Bone);
};

} // namespace riistudio::g3d
