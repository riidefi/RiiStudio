#include "CameraController.hpp"
#include <core/common.h>
#include <glm/gtx/euler_angles.hpp>
#include <imgui/imgui.h>

IMPORT_STD;

namespace riistudio::frontend {

constexpr float MIN_SPEED = 10.0f;
constexpr float MAX_SPEED = 1000.f;
constexpr float SCROLL_SPEED = 10.0f;

void CameraController::move(float time_step, InputState input) {
  const auto pos = input.mouse.has_value() ? input.mouse->position
                                           : glm::vec2(mPrevX, mPrevY);

  const float x_delta = pos.x - mPrevX;
  const float y_delta = pos.y - mPrevY;
  mPrevX = pos.x;
  mPrevY = pos.y;
#ifdef CAMERA_CONTROLLER_DEBUG
  ImGui::Text("Mouse position: %f %f. Last: %f %f. Delta: %f %f", pos.x, pos.y,
              mPrevX, mPrevY, x_delta, y_delta);
#endif
  const float horiz_delta = mMouseSpeed * time_step * -x_delta;
  const float vert_delta = mMouseSpeed * time_step * -y_delta;
#ifdef CAMERA_CONTROLLER_DEBUG
  ImGui::Text("Horiz delta: %f (%f). Vert delta: %f (%f)", horiz_delta,
              glm::degrees(horiz_delta), vert_delta, glm::degrees(vert_delta));
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif
  if (std::isnan(mHorizontalAngle) || std::isnan(mVerticalAngle)) {
    mHorizontalAngle = 3.14f;
    mVerticalAngle = 0.0f;
  }
  if (input.clickView) {
    mHorizontalAngle += horiz_delta;
    mVerticalAngle += vert_delta;

    // Slightly less than pi/2 prevents weird behavior when parallel with up
    // direction
    mVerticalAngle = std::clamp(mVerticalAngle, -1.56f, 1.56f);
  }
#ifdef CAMERA_CONTROLLER_DEBUG
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  mCamera.mDirection = glm::vec3(cos(mVerticalAngle) * sin(mHorizontalAngle),
                                 sin(mVerticalAngle),
                                 cos(mVerticalAngle) * cos(mHorizontalAngle));
  const float y_movement = combo_choice_cam == ControllerType::WASD_Minecraft
                               ? 0.0f
                               : mCamera.mDirection.y;

  glm::vec3 mvmt_dir{mCamera.mDirection.x, y_movement, mCamera.mDirection.z};
  if (combo_choice_cam == ControllerType::WASD_Minecraft)
    mvmt_dir = glm::normalize(mvmt_dir);
  glm::vec3 right =
      glm::vec3(sin(mHorizontalAngle - 1.57), 0, cos(mHorizontalAngle - 1.57));

  // NOTE: Only scroll if clicked. This prevents scrolls from outside the
  // viewport affecting the speed accidentally.
  // TODO: Check if we're hovering over the window, rather than just check
  // clicking.
  if (input.mouse.has_value() && input.clickView) {
    mSpeed += input.mouse->scroll * SCROLL_SPEED;

    // Prevents inversion of controls when speed is negative.
    if (mSpeed <= 0.f)
      mSpeed = 0.f;
    // mSpeed = std::clamp(mSpeed, MIN_SPEED, MAX_SPEED);
  }

  if (input.forward)
    mCamera.mEye += mvmt_dir * time_step * mSpeed * mSpeedFactor;
  if (input.backward)
    mCamera.mEye -= mvmt_dir * time_step * mSpeed * mSpeedFactor;
  if (input.left)
    mCamera.mEye -= right * time_step * mSpeed * mSpeedFactor;
  if (input.right)
    mCamera.mEye += right * time_step * mSpeed * mSpeedFactor;
  if (input.up)
    mCamera.mEye += mCamera.getUp() * time_step * mSpeed * mSpeedFactor;
  if (input.down)
    mCamera.mEye -= mCamera.getUp() * time_step * mSpeed * mSpeedFactor;
}

void CameraController::drawOptions() {
  drawProjectionOption();

  ImGui::SliderFloat("Mouse Speed"_j, &mMouseSpeed, 0.0f, .2f);
  ImGui::SliderFloat("Camera Speed"_j, &mSpeed, MIN_SPEED, MAX_SPEED);
  ImGui::SliderFloat("FOV", &mCamera.mFOV, 1.0f, 180.0f);

  ImGui::InputFloat("Near Plane"_j, &mCamera.mClipMin);
  ImGui::InputFloat("Far Plane"_j, &mCamera.mClipMax);

  ImGui::DragFloat("X", &mCamera.mEye.x, .01f, -10, 30);
  ImGui::DragFloat("Y", &mCamera.mEye.y, .01f, -10, 30);
  ImGui::DragFloat("Z", &mCamera.mEye.z, .01f, -10, 30);

  drawControllerTypeOption();
}

void CameraController::drawControllerTypeOption() {
  ImGui::Combo("##Controls", (int*)&combo_choice_cam,
               "WASD // FPS\0WASD // Plane\0");
}

void CameraController::drawProjectionOption() {
  ImGui::Combo("##Projection", (int*)&mCamera.mProjection,
               "Perspective\0Orthographic (Parallel)\0");
}

void SetCameraControllerToMatrix(CameraController& controller,
                                 const glm::mat4& view) {
  auto cartesian = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f) * view);
  assert(!std::isnan(cartesian.x) && !std::isnan(cartesian.y) &&
         !std::isnan(cartesian.z));

#ifdef CAMERA_CONTROLLER_DEBUG
  {
    riistudio::util::ConditionalHighlight g(true);
    ImGui::Text("Cartesian: %f, %f, %f", cartesian.x, cartesian.y, cartesian.z);
  }
#endif
  {

    auto d = glm::distance(cartesian, glm::vec3(0.0f, 0.0f, 0.0f));

    controller.mHorizontalAngle = atan2f(cartesian.x, cartesian.z);
    controller.mVerticalAngle = asinf(cartesian.y / d);
  }
}

} // namespace riistudio::frontend
