#pragma once

#include "Bone.hpp"
#include "Material.hpp"
#include "Polygon.hpp"
#include "Texture.hpp"

#include <memory> // std::shared_ptr

namespace riistudio::lib3d {

struct SceneState;

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
  virtual void prepare(const kpi::IDocumentNode& root) = 0;
};

// TODO: This should all be runtime
struct Scene : public IDrawable {
  virtual ~Scene() = default;
  Scene();

  void draw() override;
  void build(const glm::mat4& view, const glm::mat4& proj,
             AABB& bound) override;
  void prepare(const kpi::IDocumentNode& host) override;

private:
  std::shared_ptr<SceneState> mState;
};
struct Model {
  virtual ~Model() = default;
};

} // namespace riistudio::lib3d
