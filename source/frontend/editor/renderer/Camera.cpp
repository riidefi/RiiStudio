#include "Camera.hpp"
#include <Imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace riistudio::frontend {

void Camera::calc(bool showCursor, float mouseSpeed, int combo_choice_cam,
                  float width, float height, glm::mat4& projMtx,
                  glm::mat4& viewMtx, bool _w, bool _a, bool _s, bool _d,
                  bool _up, bool _down) {

  // horizontal angle : toward -Z
  float horizontalAngle = 3.14f;
  // vertical angle : 0, look at the horizon
  float verticalAngle = 0.0f;

  // TODO: Enable for plane mode
  // glm::vec3 up = glm::cross(right, direction);
  glm::vec3 up = {0, 1, 0};

  float deltaTime = 1.0f / ImGui::GetIO().Framerate;

  static float xpos = 0;
  static float ypos = 0;

  if (!showCursor) {
    auto pos = ImGui::GetMousePos();

    xpos = pos.x;
    ypos = pos.y;
  }
  horizontalAngle += mouseSpeed * deltaTime * float(width - xpos);
  verticalAngle += mouseSpeed * deltaTime * float(height - ypos);

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  direction =
      glm::vec3(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle),
                cos(verticalAngle) * cos(horizontalAngle));
  glm::vec3 mvmt_dir{direction.x, combo_choice_cam == 0 ? 0.0f : direction.y,
                     direction.z};
  if (combo_choice_cam == 0)
    mvmt_dir = glm::normalize(mvmt_dir);
  glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0,
                              cos(horizontalAngle - 3.14f / 2.0f));

  projMtx = glm::perspective(
      glm::radians(fov), static_cast<float>(width) / static_cast<float>(height),
      cmin, cmax);
  viewMtx = glm::lookAt(eye, eye + direction, up);

  if (_w)
    eye += mvmt_dir * deltaTime * speed;
  if (_s)
    eye -= mvmt_dir * deltaTime * speed;
  if (_a)
    eye -= right * deltaTime * speed;
  if (_d)
    eye += right * deltaTime * speed;
  if (_up)
    eye += up * deltaTime * speed;
  if (_down)
    eye -= up * deltaTime * speed;
}

void Camera::drawOptions() {
  ImGui::SliderFloat("Camera Speed", &speed, 1.0f, 10000.f);
  ImGui::SliderFloat("FOV", &fov, 1.0f, 180.0f);

  ImGui::InputFloat("Near Plane", &cmin);
  ImGui::InputFloat("Far Plane", &cmax);

  ImGui::DragFloat("X", &eye.x, .01f, -10, 30);
  ImGui::DragFloat("Y", &eye.y, .01f, -10, 30);
  ImGui::DragFloat("Z", &eye.z, .01f, -10, 30);
}

} // namespace riistudio::frontend
