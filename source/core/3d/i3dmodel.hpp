#pragma once

#include "Bone.hpp"
#include "Material.hpp"
#include "Polygon.hpp"
#include "Texture.hpp"
#include <core/kpi/Node2.hpp> // kpi::Collection
#include <librii/glhelper/VBOBuilder.hpp>
#include <memory> // std::shared_ptr

namespace riistudio::lib3d {

struct SceneState;
class Scene;

class DrawableDispatcher;

// Used for the root element
struct IDrawable {
  virtual ~IDrawable() = default;

  //! Prepare a scene based on the resource data.
  virtual void prepare(SceneState& state, const kpi::INode& root,
                       glm::mat4 v_mtx, glm::mat4 p_mtx) = 0;

  DrawableDispatcher& getDispatcher() {
    assert(dispatcher);
    return *dispatcher;
  }

  DrawableDispatcher* dispatcher = nullptr;
};

class DrawableDispatcher {
public:
  void beginEdit() { poisoned = true; }
  void endEdit() {
    poisoned = false;
    reinit = true;
  }

  void populate(IDrawable& drawable, SceneState& state, const kpi::INode& root,
                glm::mat4 v_mtx, glm::mat4 p_mtx) {
    assert(!poisoned);
    assert(!reinit);
    drawable.prepare(state, root, v_mtx, p_mtx);
  }

  bool beginDraw() {
    if (poisoned)
      return false;

    // We don't have any initialization code yet
    reinit = false;

    return true;
  }

  void endDraw() {}

private:
  bool poisoned = false;
  bool reinit = false;
};

struct SceneBuffers;

struct SceneImpl : public IDrawable {
  virtual ~SceneImpl();
  SceneImpl();

  void prepare(SceneState& state, const kpi::INode& host, glm::mat4 v_mtx,
               glm::mat4 p_mtx) override;

  void gatherBoneRecursive(SceneBuffers& output, u64 boneId,
                           const lib3d::Model& root, const lib3d::Scene& scn,
                           glm::mat4 v_mtx, glm::mat4 p_mtx);

  void gather(SceneBuffers& output, const lib3d::Model& root,
              const lib3d::Scene& scene, glm::mat4 v_mtx, glm::mat4 p_mtx);

private:
  struct Internal;
  std::unique_ptr<Internal> mImpl;
};

} // namespace riistudio::lib3d

#include "Node.h"
