#include "Camera.hpp"
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace riistudio::frontend {

void Camera::calc(bool showCursor, float mouseSpeed, int combo_choice_cam,
                  float width, float height, glm::mat4& projMtx,
                  glm::mat4& viewMtx, bool _w, bool _a, bool _s, bool _d,
                  bool _up, bool _down) {

  // TODO: Enable for plane mode
  // glm::vec3 up = glm::cross(right, direction);
  glm::vec3 up = {0, 1, 0};

  float deltaTime = 1.0f / ImGui::GetIO().Framerate;

  const auto pos = ImGui::GetMousePos();

  const float x_delta = pos.x - xPrev;
  const float y_delta = pos.y - yPrev;
  xPrev = pos.x;
  yPrev = pos.y;
#ifdef BUILD_DEBUG
  ImGui::Text("Mouse position: %f %f. Last: %f %f. Delta: %f %f", pos.x, pos.y,
              xPrev, yPrev, x_delta, y_delta);
#endif
  const float horiz_delta = mouseSpeed * deltaTime * -x_delta;
  const float vert_delta = mouseSpeed * deltaTime * -y_delta;
#ifdef BUILD_DEBUG
  ImGui::Text("Horiz delta: %f (%f). Vert delta: %f (%f)", horiz_delta,
              horiz_delta * 180.0f / 3.1415f, vert_delta,
              vert_delta * 180.0f / 3.1415f);
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", horizontalAngle,
              glm::degrees(horizontalAngle), verticalAngle,
              glm::degrees(verticalAngle));
#endif
  // Not sure why this happens
  if (std::abs(horizontalAngle) > 100000.0f) horizontalAngle = 0.0f;
  if (std::abs(verticalAngle) > 100000.0f) verticalAngle = 0.0f;
#ifdef BUILD_DEBUG
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", horizontalAngle,
	  glm::degrees(horizontalAngle), verticalAngle,
	  glm::degrees(verticalAngle));
#endif
  if (!showCursor)
  {
    horizontalAngle += horiz_delta;
    verticalAngle += vert_delta;
  }
#ifdef BUILD_DEBUG
  ImGui::Text("Horiz %f (%f degrees), Vert %f (%f degrees)", horizontalAngle,
	  glm::degrees(horizontalAngle), verticalAngle,
	  glm::degrees(verticalAngle));
#endif
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
