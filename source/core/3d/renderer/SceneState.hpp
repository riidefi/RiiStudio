#pragma once

#include <core/3d/aabb.hpp>               // AABB
#include <core/3d/renderer/SceneTree.hpp> // SceneTree
#include <librii/glhelper/UBOBuilder.hpp> // DelegatedUBOBuilder
#include <librii/glhelper/VBOBuilder.hpp> // VBOBuilder

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

  librii::glhelper::VBOBuilder mVbo;
  struct Texture {
    u32 id;
    // std::vector<u8> data;
    //	u32 width;
    //	u32 height;
  };
  std::vector<Texture> mTextures;
  librii::glhelper::DelegatedUBOBuilder mUboBuilder;
};

} // namespace riistudio::lib3d
