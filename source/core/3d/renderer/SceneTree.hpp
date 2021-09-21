#pragma once

#include <core/3d/i3dmodel.hpp>
#include <cstring>
#include <librii/gfx/SceneNode.hpp>
#include <librii/gfx/TextureObj.hpp>
#include <librii/glhelper/ShaderCache.hpp>
#include <librii/glhelper/UBOBuilder.hpp>
#include <llvm/ADT/SmallVector.h>
#include <map>
#include <rsl/ArrayVector.hpp>

namespace riistudio::lib3d {

// Not implemented yet
enum class RenderPass {
  Standard,
  ID // For selection
};

struct DrawBuffer {
  std::vector<librii::gfx::SceneNode> nodes;

  auto begin() { return nodes.begin(); }
  auto begin() const { return nodes.begin(); }
  auto end() { return nodes.end(); }
  auto end() const { return nodes.end(); }

  void zSort() {
    // FIXME: Implement
  }
};

struct SceneBuffers {
  using Node = librii::gfx::SceneNode;

  DrawBuffer opaque;
  DrawBuffer translucent;

  template <typename T> void forEachNode(T functor) {
    for (auto& node : opaque)
      functor(node);
    for (auto& node : translucent)
      functor(node);
  }
};

} // namespace riistudio::lib3d
