#pragma once

#include <glm/glm.hpp>

namespace riistudio::frontend {

class Camera {
public:
  void calc(bool showCursor, float mouseSpeed, int combo_choice_cam,
            float width, float height, glm::mat4& projMtx, glm::mat4& viewMtx,
            bool w, bool a, bool s, bool d, bool up, bool down);

  void drawOptions();

  float getSpeed() const { return mSpeed; }
  void setSpeed(float s) { mSpeed = s; }

  glm::vec3 getPosition() { return mEye; }
  void setPosition(const glm::vec3& p) { mEye = p; }

  void setClipPlanes(float near, float far) {
    mClipMin = near;
    mClipMax = far;
  }

private:
  // Camera settings
  float mSpeed = 0.0f;
  float mClipMin = 1.0f;
  float mClipMax = 50000.f;
  float mFOV = 90.0f;
  // Camera state
  glm::vec3 mEye{0.0f, 0.0f, 0.0f};
  glm::vec3 mDirection;
  float mHorizontalAngle = 3.14f;
  float mVerticalAngle = 0.0f;
  float mPrevX = 0.0f;
  float mPrevY = 0.0f;
};

} // namespace riistudio::frontend
