#pragma once

#include <array>
#include <core/common.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <librii/kcol/Model.hpp>
#include <vector>

#include <core/3d/renderer/SceneState.hpp>
#include <librii/glhelper/VBOBuilder.hpp>

namespace riistudio::lvl {

struct Triangle {
  u16 attr;
  std::array<glm::vec3, 3> verts;
};

class TriangleRenderer {
public:
  // Upload initial triangle data to GPU
  void init(const librii::kcol::KCollisionData& mCourseKcl);

  // Z-Sort triangles on CPU, upload to GPU
  void sortTriangles(const glm::mat4& viewMtx);

  // Add draw call to tree
  void draw(riistudio::lib3d::SceneState& state, const glm::mat4& modelMtx,
            const glm::mat4& viewMtx, const glm::mat4& projMtx, u32 attr_mask,
            float alpha);

private:
  // Take prism-form indexed vectors from `kcl`, and populate mKclTris
  void convertToTriangles(const librii::kcol::KCollisionData& kcl);

  // Populate tri_vbo by mKclTris
  void buildVertexBuffer();

  std::vector<Triangle> mKclTris;
  // KCL triangle vertex buffer
  std::unique_ptr<librii::glhelper::VBOBuilder> tri_vbo = nullptr;
  // z_dist of every kcl triangle and the camera
  std::vector<float> z_dist;
};

} // namespace riistudio::lvl
