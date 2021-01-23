#pragma once

#include <core/3d/aabb.hpp>               // AABB
#include <core/3d/renderer/GlTexture.hpp> // GlTexture
#include <core/3d/renderer/SceneTree.hpp> // SceneBuffers
#include <librii/glhelper/UBOBuilder.hpp> // DelegatedUBOBuilder
#include <librii/glhelper/VBOBuilder.hpp> // VBOBuilder

namespace riistudio::lib3d {

struct SceneState {
public:
  SceneState() = default;
  ~SceneState() = default;

  // Upload a texture to the database
  void addTexture(const std::string& key, const lib3d::Texture& tex);

  // Compute the composite bounding box (in model space)
  AABB computeBounds();

  // Build the UBO. Typically called every frame.
  void buildUniformBuffers(const glm::mat4& view, const glm::mat4& proj);

  // Draw the model to the screen. You'll want to clear it first.
  void draw();

  // Direct access to attached renderables.
  SceneBuffers& getBuffers() { return mTree; }

private:
  SceneBuffers mTree;
  std::map<std::string, u32> texIdMap;

  std::vector<GlTexture> mTextures;
  librii::glhelper::DelegatedUBOBuilder mUboBuilder;
};

} // namespace riistudio::lib3d
