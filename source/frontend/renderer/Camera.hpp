#pragma once

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace riistudio::frontend {

class Camera {
  friend class CameraController;

public:
  void calcMatrices(float width, float height, glm::mat4& projMtx,
                    glm::mat4& viewMtx);

  glm::vec3 getPosition() { return mEye; }
  void setPosition(const glm::vec3& p) { mEye = p; }

  void setClipPlanes(float near_, float far_) {
    mClipMin = near_;
    mClipMax = far_;
  }

  // TODO: Enable for plane mode
  // glm::vec3 up = glm::cross(right, direction);
  glm::vec3 getUp() const { return {0, 1, 0}; }

private:
  // Camera settings
  float mClipMin = 1.0f;
  float mClipMax = 50000.f;
  float mFOV = 90.0f;
  // Camera state
  glm::vec3 mEye{0.0f, 0.0f, 0.0f};
  glm::vec3 mDirection;
};

} // namespace riistudio::frontend
