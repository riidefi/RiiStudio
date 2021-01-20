#pragma once

#include "Bone.hpp"
#include "Material.hpp"
#include "Polygon.hpp"
#include "Texture.hpp"
#include "aabb.hpp"
#include <core/kpi/Node2.hpp>             // kpi::Collection
#include <memory>                         // std::shared_ptr

namespace riistudio::lib3d {

struct SceneState;
class Scene;

// Used for the root element
struct IDrawable {
  virtual ~IDrawable() = default;

  //! Draw the entire scene.
  virtual void draw() = 0;

  //! Build the scene state given the view and projection matrices.
  //! Output the bounding box
  virtual void build(const glm::mat4& view, const glm::mat4& proj,
                     AABB& bound) = 0;

  //! Prepare a scene based on the resource data.
  virtual void prepare(const kpi::INode& root) = 0;

  bool poisoned = false;
  bool reinit = false;
};

struct SceneNode;

struct SceneImpl : public IDrawable {
  virtual ~SceneImpl() = default;
  SceneImpl();

  void drawNode(SceneNode& node);
  void draw() override;
  void build(const glm::mat4& view, const glm::mat4& proj,
             AABB& bound) override;
  void prepare(const kpi::INode& host) override;

private:
  std::shared_ptr<SceneState> mState;
};

} // namespace riistudio::lib3d

#include "Node.h"
