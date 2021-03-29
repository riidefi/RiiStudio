#pragma once

#include <array>
#include <core/common.h>
#include <glm/vec3.hpp>
#include <librii/math/aabb.hpp>
#include <string>
#include <vector>

namespace librii::j3d {

enum class MatrixType : u16 { Standard = 0, Billboard, BillboardY };

struct JointData {
  std::string name = "root";

  u16 flag = 1; // Unused four bits; default value in galaxy is 1
  MatrixType bbMtxType = MatrixType::Standard;
  bool mayaSSC = false;

  glm::vec3 scale{1.0f, 1.0f, 1.0f};
  glm::vec3 rotate{0.0f, 0.0f, 0.0f};
  glm::vec3 translate{0.0f, 0.0f, 0.0f};

  f32 boundingSphereRadius = 100000.0f;
  librii::math::AABB boundingBox{{-100000.0f, -100000.0f, -100000.0f},
                                 {100000.0f, 100000.0f, 100000.0f}};

  s32 parentId = -1;
  std::vector<s32> children;

  struct Display {
    s32 material = 0;
    s32 shape = 0;

    Display() = default;
    Display(s32 mat, s32 shp) : material(mat), shape(shp) {}
    bool operator==(const Display& rhs) const = default;
  };
  std::vector<Display> displays;

  // glm::mat4x4
  std::array<float, 12> inverseBindPoseMtx;

  bool operator==(const JointData& rhs) const = default;
};

} // namespace librii::j3d
