#pragma once

#include <core/3d/Bone.hpp>
#include <core/3d/Material.hpp>
#include <core/3d/Polygon.hpp>
#include <core/3d/Texture.hpp>
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
  virtual Result<void> prepare(SceneState& state, const Scene& root,
                               glm::mat4 v_mtx, glm::mat4 p_mtx,
                               RenderType type) = 0;

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

  Result<void> populate(IDrawable& drawable, SceneState& state,
                        const lib3d::Scene& root, glm::mat4 v_mtx,
                        glm::mat4 p_mtx, RenderType type) {
    assert(!poisoned);
    assert(!reinit);
    return drawable.prepare(state, root, v_mtx, p_mtx, type);
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

} // namespace riistudio::lib3d

#include "Node.h"
