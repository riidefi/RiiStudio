#pragma once

#include <assimp/color4.h>
#include <assimp/matrix4x4.h>
#include <assimp/vector2.h>
#include <assimp/vector3.h>
#include <glm/glm.hpp>
#include <plugins/gc/GX/Material.hpp>
#include <string>

namespace riistudio::ass {

static inline std::string getFileShort(const std::string& path) {
  auto tmp = path.substr(path.rfind("\\") + 1);
  tmp = tmp.substr(0, tmp.rfind("."));
  return tmp;
}
static inline glm::vec3 getVec(const aiVector3D& vec) {
  return {vec.x, vec.y, vec.z};
}
static inline glm::vec2 getVec2(const aiVector3D& vec) {
  return {vec.x, vec.y};
}
static inline libcube::gx::Color getClr(const aiColor4D& clr) {
  libcube::gx::ColorF32 fclr{clr.r, clr.g, clr.b, clr.a};
  return fclr;
}
static inline glm::mat4 getMat4(const aiMatrix4x4& mtx) {
  glm::mat4 out;

  out[0][0] = mtx.a1;
  out[1][0] = mtx.a2;
  out[2][0] = mtx.a3;
  out[3][0] = mtx.a4;
  out[0][1] = mtx.b1;
  out[1][1] = mtx.b2;
  out[2][1] = mtx.b3;
  out[3][1] = mtx.b4;
  out[0][2] = mtx.c1;
  out[1][2] = mtx.c2;
  out[2][2] = mtx.c3;
  out[3][2] = mtx.c4;
  out[0][3] = mtx.d1;
  out[1][3] = mtx.d2;
  out[2][3] = mtx.d3;
  out[3][3] = mtx.d4;
  return out;
}

} // namespace riistudio::ass
