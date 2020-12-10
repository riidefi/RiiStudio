#include "CameraController.hpp"
#include <algorithm>
#include <glm/gtx/euler_angles.hpp>
#include <imgui/imgui.h>

namespace riistudio::frontend {

constexpr float MIN_SPEED = 10.0f;
constexpr float MAX_SPEED = 1000.f;
constexpr float SCROLL_SPEED = 10.0f;

void CameraController::move(float mouseSpeed, int combo_choice_cam, InputState input) {

  float deltaTime = 1.0f / ImGui::GetIO().Framerate;

  const auto pos =
      ImGui::IsMousePosValid() ? ImGui::GetMousePos() : ImVec2(mPrevX, mPrevY);

  const float x_delta = pos.x - mPrevX;
  const float y_delta = pos.y - mPrevY;
  mPrevX = pos.x;
  mPrevY = pos.y;
#ifdef BUILD_DEBUG
  ImGui::Text("Mouse position: %f %f. Last: %f %f. Delta: %f %f", pos.x, pos.y,
              mPrevX, mPrevY, x_delta, y_delta);
#endif
  const float horiz_delta = mouseSpeed * deltaTime * -x_delta;
  const float vert_delta = mouseSpeed * deltaTime * -y_delta;
#ifdef BUILD_DEBUG
  ImGui::Text("Horiz delta: %f (%f). Vert delta: %f (%f)", horiz_delta,
              glm::degrees(horiz_delta), vert_delta, glm::degrees(vert_delta));
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif

#ifdef BUILD_DEBUG
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif
  if (input.clickView) {
    mHorizontalAngle += horiz_delta;
    mVerticalAngle += vert_delta;
    // Slightly less than pi/2 prevents weird behavior when parallel with up
    // direction
    mVerticalAngle = std::clamp(mVerticalAngle, -1.56f, 1.56f);
  }
#ifdef BUILD_DEBUG
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  mCamera.mDirection = glm::vec3(cos(mVerticalAngle) * sin(mHorizontalAngle),
                         sin(mVerticalAngle),
                         cos(mVerticalAngle) * cos(mHorizontalAngle));
  glm::vec3 mvmt_dir{mCamera.mDirection.x, combo_choice_cam == 0 ? 0.0f : mCamera.mDirection.y,
                     mCamera.mDirection.z};
  if (combo_choice_cam == 0)
    mvmt_dir = glm::normalize(mvmt_dir);
  glm::vec3 right =
      glm::vec3(sin(mHorizontalAngle - 1.57), 0, cos(mHorizontalAngle - 1.57));

  mSpeed += ImGui::GetIO().MouseWheel * SCROLL_SPEED;
  mSpeed = std::clamp(mSpeed, MIN_SPEED, MAX_SPEED);

  if (input.forward)
    mCamera.mEye += mvmt_dir * deltaTime * mSpeed * mSpeedFactor;
  if (input.backward)
    mCamera.mEye -= mvmt_dir * deltaTime * mSpeed * mSpeedFactor;
  if (input.left)
    mCamera.mEye -= right * deltaTime * mSpeed * mSpeedFactor;
  if (input.right)
    mCamera.mEye += right * deltaTime * mSpeed * mSpeedFactor;
  if (input.up)
    mCamera.mEye += mCamera.getUp() * deltaTime * mSpeed * mSpeedFactor;
  if (input.down)
    mCamera.mEye -= mCamera.getUp() * deltaTime * mSpeed * mSpeedFactor;
}

void CameraController::drawOptions() {
  ImGui::SliderFloat("Camera Speed", &mSpeed, MIN_SPEED, MAX_SPEED);
  ImGui::SliderFloat("FOV", &mCamera.mFOV, 1.0f, 180.0f);

  ImGui::InputFloat("Near Plane", &mCamera.mClipMin);
  ImGui::InputFloat("Far Plane", &mCamera.mClipMax);

  ImGui::DragFloat("X", &mCamera.mEye.x, .01f, -10, 30);
  ImGui::DragFloat("Y", &mCamera.mEye.y, .01f, -10, 30);
  ImGui::DragFloat("Z", &mCamera.mEye.z, .01f, -10, 30);
}
}
