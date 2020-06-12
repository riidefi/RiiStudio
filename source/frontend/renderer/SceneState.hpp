#pragma once

#include "SceneTree.hpp"
#include <core/3d/aabb.hpp>

namespace riistudio::frontend {

struct SceneState {
  ~SceneState();

  void buildBuffers();

  void buildTextures(const kpi::IDocumentNode& root);
  void gather(const kpi::IDocumentNode& model,
              const kpi::IDocumentNode& texture, bool buf = true,
              bool tex = true);

  void build(const glm::mat4& view, const glm::mat4& proj,
             riistudio::lib3d::AABB& bound);

  void draw();

  DelegatedUBOBuilder mUboBuilder;
  SceneTree mTree;
  std::map<std::string, u32> texIdMap;
  const kpi::FolderData* bones;
  glm::mat4 scaleMatrix{1.0f};

  VBOBuilder mVbo;
  struct Texture {
    u32 id;
    // std::vector<u8> data;
    //	u32 width;
    //	u32 height;
  };
  std::vector<Texture> mTextures;
};

} // namespace riistudio::frontend
