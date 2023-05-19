#pragma once

#include "OutputCtx.hpp"
#include <oishii/reader/binary_reader.hxx>

namespace oishii {
class Node;
}

namespace librii::j3d {

struct SceneGraph {
  static constexpr const char name[] = "SceneGraph";

  [[nodiscard]] static Result<void> onRead(rsl::SafeReader& reader,
                                           BMDOutputContext& ctx);
  static std::unique_ptr<oishii::Node> getLinkerNode(const J3dModel& mdl);
};

} // namespace librii::j3d
