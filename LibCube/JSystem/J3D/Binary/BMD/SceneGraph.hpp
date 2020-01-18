#pragma once

#include "oishii/types.hxx"
#include <oishii/reader/binary_reader.hxx>
#include "BMD.hpp"

namespace oishii { struct Node; }

namespace libcube::jsystem {

struct SceneGraph
{
	static constexpr const char name[] = "SceneGraph";

	static void onRead(oishii::BinaryReader& reader, BMDOutputContext& ctx);
	static oishii::Node* getLinkerNode(const J3DModel& mdl, bool linkerOwned=true);
};

} // namespace libcube::jsystem
