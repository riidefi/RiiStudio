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
	static std::unique_ptr<oishii::v2::Node> getLinkerNode(const J3DModel& mdl);
};

} // namespace libcube::jsystem
