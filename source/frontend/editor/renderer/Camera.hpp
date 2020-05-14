#pragma once

#include <glm/glm.hpp>

namespace riistudio::frontend {

class Camera {
public:
  void calc(bool showCursor, float mouseSpeed, int combo_choice_cam,
            float width, float height, glm::mat4& projMtx, glm::mat4& viewMtx,
            bool w, bool a, bool s, bool d, bool up, bool down);
  void drawOptions();

  float getSpeed() const { return speed; }
  void setSpeed(float s) { speed = s; }

  glm::vec3 getPosition() { return eye; }
  void setPosition(const glm::vec3& p) { eye = p; }

private:
  // Camera settings
  float speed = 0.0f;
  float cmin = 1.0f;
  float cmax = 50000.f;
  float fov = 90.0f;
  // Camera state
  glm::vec3 eye{0.0f, 0.0f, 0.0f};
  glm::vec3 direction;
};

} // namespace riistudio::frontend
