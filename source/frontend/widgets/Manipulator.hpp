#pragma once

#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>

// Include order matters
#include <vendor/ImGuizmo.h>

class Manipulator {
public:
  Manipulator() = default;
  // No move/copy: snap is a pointer
  Manipulator(const Manipulator&) = delete;
  Manipulator(Manipulator&&) = delete;

  void drawUi(glm::mat4& mx);
  bool manipulate(glm::mat4& mx, const glm::mat4& viewMtx,
                  const glm::mat4& projMtx);
  void drawDebugCube(const glm::mat4& mx, const glm::mat4& viewMtx,
                     const glm::mat4& projMtx);

private:
  ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;
  bool useSnap = false;
  float st[3];
  float sr[3];
  float ss[3];
  float* snap = st;
};
