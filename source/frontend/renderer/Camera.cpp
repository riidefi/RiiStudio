#include "Camera.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imgui/imgui.h>

namespace riistudio::frontend {

constexpr float MAX_SPEED = 10000.f;

void Camera::calc(bool showCursor, float mouseSpeed, int combo_choice_cam,
                  float width, float height, glm::mat4& projMtx,
                  glm::mat4& viewMtx, bool _w, bool _a, bool _s, bool _d,
                  bool _up, bool _down) {

  // TODO: Enable for plane mode
  // glm::vec3 up = glm::cross(right, direction);
  glm::vec3 up = {0, 1, 0};

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
              horiz_delta * 180.0f / 3.1415f, vert_delta,
              vert_delta * 180.0f / 3.1415f);
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif

#ifdef BUILD_DEBUG
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif
  if (!showCursor) {
    mHorizontalAngle += horiz_delta;
    mVerticalAngle += vert_delta;
  }
#ifdef BUILD_DEBUG
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", mHorizontalAngle,
              glm::degrees(mHorizontalAngle), mVerticalAngle,
              glm::degrees(mVerticalAngle));
#endif

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  mDirection = glm::vec3(cos(mVerticalAngle) * sin(mHorizontalAngle),
                         sin(mVerticalAngle),
                         cos(mVerticalAngle) * cos(mHorizontalAngle));
  glm::vec3 mvmt_dir{mDirection.x, combo_choice_cam == 0 ? 0.0f : mDirection.y,
                     mDirection.z};
  if (combo_choice_cam == 0)
    mvmt_dir = glm::normalize(mvmt_dir);
  glm::vec3 right = glm::vec3(sin(mHorizontalAngle - 3.14f / 2.0f), 0,
                              cos(mHorizontalAngle - 3.14f / 2.0f));

  projMtx =
      glm::perspective(glm::radians(mFOV),
                       static_cast<float>(width) / static_cast<float>(height),
                       mClipMin, mClipMax);
  viewMtx = glm::lookAt(mEye, mEye + mDirection, up);

  if (_w)
    mEye += mvmt_dir * deltaTime * mSpeed;
  if (_s)
    mEye -= mvmt_dir * deltaTime * mSpeed;
  if (_a)
    mEye -= right * deltaTime * mSpeed;
  if (_d)
    mEye += right * deltaTime * mSpeed;
  if (_up)
    mEye += up * deltaTime * mSpeed;
  if (_down)
    mEye -= up * deltaTime * mSpeed;
}

void Camera::drawOptions() {
  ImGui::SliderFloat("Camera Speed", &mSpeed, 1.0f, MAX_SPEED);
  ImGui::SliderFloat("FOV", &mFOV, 1.0f, 180.0f);

  ImGui::InputFloat("Near Plane", &mClipMin);
  ImGui::InputFloat("Far Plane", &mClipMax);

  ImGui::DragFloat("X", &mEye.x, .01f, -10, 30);
  ImGui::DragFloat("Y", &mEye.y, .01f, -10, 30);
  ImGui::DragFloat("Z", &mEye.z, .01f, -10, 30);
}

} // namespace riistudio::frontend
