#pragma once

#include "BMD.hpp"
#include "oishii/types.hxx"
#include <oishii/reader/binary_reader.hxx>

namespace oishii {
class Node;
}

namespace riistudio::j3d {

struct SceneGraph {
  static constexpr const char name[] = "SceneGraph";

  static void onRead(oishii::BinaryReader &reader, BMDOutputContext &ctx);
  static std::unique_ptr<oishii::v2::Node>
  getLinkerNode(const ModelAccessor mdl);
};

} // namespace riistudio::j3d
