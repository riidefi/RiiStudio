#pragma once

#include "Bone.hpp"
#include "Material.hpp"
#include "Polygon.hpp"
#include "Texture.hpp"
#include "aabb.hpp"
#include <core/kpi/Node2.hpp> // kpi::Collection
#include <librii/glhelper/VBOBuilder.hpp>
#include <memory> // std::shared_ptr

namespace riistudio::lib3d {

struct SceneState;
class Scene;

// Used for the root element
struct IDrawable {
  virtual ~IDrawable() = default;

  //! Prepare a scene based on the resource data.
  virtual void prepare(SceneState& state, const kpi::INode& root) = 0;

  bool poisoned = false;
  bool reinit = false;
};

struct SceneNode;
struct SceneBuffers;

struct SceneImpl : public IDrawable {
  virtual ~SceneImpl() = default;
  SceneImpl();

  void prepare(SceneState& state, const kpi::INode& host) override;

  void gatherBoneRecursive(SceneBuffers& output, u64 boneId,
                           const lib3d::Model& root, const lib3d::Scene& scn);

  void gather(SceneBuffers& output, const lib3d::Model& root,
              const lib3d::Scene& scene);

  std::unique_ptr<librii::glhelper::VBOBuilder> mVboBuilder;
};

} // namespace riistudio::lib3d

#include "Node.h"
