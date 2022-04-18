#include "Manipulator.hpp"
#include <glm/gtc/type_ptr.hpp>

void Manipulator::drawUi(glm::mat4& mx) {
  if (ImGui::IsKeyPressed(ImGuiKey_G))
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  if (ImGui::IsKeyPressed(ImGuiKey_R))
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  if (ImGui::IsKeyPressed(ImGuiKey_T))
    mCurrentGizmoOperation = ImGuizmo::SCALE;
  if (ImGui::RadioButton("Translate",
                         mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
    mCurrentGizmoOperation = ImGuizmo::SCALE;
  float matrixTranslation[3], matrixRotation[3], matrixScale[3];
  ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mx), matrixTranslation,
                                        matrixRotation, matrixScale);
  ImGui::InputFloat3("Tr", matrixTranslation);
  ImGui::InputFloat3("Rt", matrixRotation);
  ImGui::InputFloat3("Sc", matrixScale);
  ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                          matrixScale, glm::value_ptr(mx));

  if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
    if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
      mCurrentGizmoMode = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
      mCurrentGizmoMode = ImGuizmo::WORLD;
  }
  if (ImGui::IsKeyPressed(83))
    useSnap = !useSnap;
  ImGui::Checkbox("SNAP", &useSnap);
  ImGui::SameLine();
  switch (mCurrentGizmoOperation) {
  case ImGuizmo::TRANSLATE:
    snap = st;
    ImGui::InputFloat3("Snap", st);
    break;
  case ImGuizmo::ROTATE:
    snap = sr;
    ImGui::InputFloat("Angle Snap", sr);
    break;
  case ImGuizmo::SCALE:
    snap = ss;
    ImGui::InputFloat("Scale Snap", ss);
    break;
  default:
    snap = st;
  }
}

bool Manipulator::manipulate(glm::mat4& mx, const glm::mat4& viewMtx,
                             const glm::mat4& projMtx) {
  return ImGuizmo::Manipulate(glm::value_ptr(viewMtx), glm::value_ptr(projMtx),
                              mCurrentGizmoOperation, mCurrentGizmoMode,
                              glm::value_ptr(mx), NULL, useSnap ? snap : NULL);
}

void Manipulator::drawDebugCube(const glm::mat4& mx, const glm::mat4& viewMtx,
                                const glm::mat4& projMtx) {
  ImGuizmo::DrawCubes(glm::value_ptr(viewMtx), glm::value_ptr(projMtx),
                      glm::value_ptr(mx), 1);
}
