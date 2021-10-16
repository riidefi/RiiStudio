#include "Transform.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <librii/math/srt3.hpp>

// This has to be included first
#include <core/util/gui.hpp>
//
#include <vendor/ImGuizmo.h>

// Returns a transformation of the unit sphere
glm::mat4 MatrixOfPoint(const glm::vec3& position, const glm::vec3& rotation,
                        int range) {
  // TODO: Scale might differ depending on point type
  return librii::math::calcXform(
      {.scale = glm::vec3(std::max(std::min(range * 50.0f, 1000.0f), 700.0f)),
       .rotation = rotation,
       .translation = position});
}

PointX PointOfMatrix(const glm::mat4& mat) {
  // NOTE: I use ImGuizmo here as ImGuizmo is going to be responsible for edits
  // to the model matrix.
  //
  // TODO: Investigate potential numerical stability issues with ImGuizmo's
  // decompose
  //
  // NOTE: glm has glm::decompose in glm/gtx/matrix_decompose.hpp
  //
  glm::vec3 matrixTranslation, matrixRotation, matrixScale;
  ImGuizmo::DecomposeMatrixToComponents(
      glm::value_ptr(mat), glm::value_ptr(matrixTranslation),
      glm::value_ptr(matrixRotation), glm::value_ptr(matrixScale));

  // TODO: Clamp range, average components
  return PointX{.pos = matrixTranslation,
                .rot = matrixRotation,
                .range = static_cast<int>(matrixScale[0] / 50.0f)};
}
