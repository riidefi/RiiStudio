#pragma once

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace riistudio::frontend {

class Camera {
public:
  void calcMatrices(float width, float height, glm::mat4& projMtx,
                    glm::mat4& viewMtx);

  struct InputState {
    bool forward : 1;
    bool left : 1;
    bool backward : 1;
    bool right : 1;
    bool up : 1;
    bool down : 1;
    bool clickSelect : 1;
    bool clickView : 1;
  };

  void calc(float mouseSpeed, int combo_choice_cam, InputState input);

  void drawOptions();

  float getSpeed() const { return mSpeed; }
  void setSpeed(float s) { mSpeed = s; }

  glm::vec3 getPosition() { return mEye; }
  void setPosition(const glm::vec3& p) { mEye = p; }

  void setSpeedFactor(float s) { mSpeedFactor = s; }

  void setClipPlanes(float near, float far) {
    mClipMin = near;
    mClipMax = far;
  }

private:
  // Camera settings
  float mSpeed = 150.0f;
  float mSpeedFactor = 0.0f;
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
