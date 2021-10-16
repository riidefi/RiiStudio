#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// Returns a transformation of the unit sphere
glm::mat4 MatrixOfPoint(const glm::vec3& position, const glm::vec3& rotation,
                        int range);
struct PointX {
  glm::vec3 pos;
  glm::vec3 rot;
  int range;
};

PointX PointOfMatrix(const glm::mat4& mat);
