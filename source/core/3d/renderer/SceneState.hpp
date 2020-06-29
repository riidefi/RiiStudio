#pragma once

#include <core/3d/aabb.hpp>                // AABB
#include <core/3d/renderer/SceneTree.hpp>  // SceneTree
#include <core/3d/renderer/UBOBuilder.hpp> // DelegatedUBOBuilder
#include <core/3d/renderer/VBOBuilder.hpp> // VBOBuilder

namespace riistudio::lib3d {

struct SceneState {
  ~SceneState();

  void buildBuffers();

  void buildTextures(const lib3d::Scene& root);
  void gather(const lib3d::Model& model, const lib3d::Scene& texture,
              bool buf = true, bool tex = true);

  void build(const glm::mat4& view, const glm::mat4& proj, AABB& bound);

  void draw();

  SceneTree mTree;
  std::map<std::string, u32> texIdMap;
  glm::mat4 scaleMatrix{1.0f};

  VBOBuilder mVbo;
  struct Texture {
    u32 id;
    // std::vector<u8> data;
    //	u32 width;
    //	u32 height;
  };
  std::vector<Texture> mTextures;
  DelegatedUBOBuilder mUboBuilder;
};

} // namespace riistudio::lib3d
