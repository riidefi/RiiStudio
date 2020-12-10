#pragma once

#include <frontend/renderer/Camera.hpp>

namespace riistudio::frontend {

class CameraController {
public:
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

  void move(float mouseSpeed, int combo_choice_cam, InputState input);

  void drawOptions();

  Camera mCamera;
  float mSpeed = 150.0f;
  float mSpeedFactor = 0.0f;
  float mHorizontalAngle = 3.14f;
  float mVerticalAngle = 0.0f;
  float mPrevX = 0.0f;
  float mPrevY = 0.0f;
};

} // namespace riistudio::frontend
