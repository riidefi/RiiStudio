#include "BoneTransformSurface.hpp"

namespace libcube::UI {

void drawProperty(kpi::PropertyDelegate<IBoneDelegate>& delegate,
                  BoneTransformSurface) {
  auto& bone = delegate.getActive();

  const auto srt = bone.getSRT();
  glm::vec3 scl = srt.scale;
  glm::vec3 rot = srt.rotation;
  glm::vec3 pos = srt.translation;

  ImGui::SliderFloat3("Scale"_j, &scl.x, 0.01f, 100.0f);
  delegate.property(
      bone.getScale(), scl, [](const auto& x) { return x.getScale(); },
      [](auto& x, const auto& y) { x.setScale(y); });

  ImGui::SliderFloat3("Rotation"_j, &rot.x, 0.0f, 360.0f);
  delegate.property(
      bone.getRotation(), rot, [](const auto& x) { return x.getRotation(); },
      [](auto& x, const auto& y) { x.setRotation(y); });
  ImGui::SliderFloat3("Translation"_j, &pos.x, -1000.0f, 1000.0f);
  delegate.property(
      bone.getTranslation(), pos,
      [](const auto& x) { return x.getTranslation(); },
      [](auto& x, const auto& y) { x.setTranslation(y); });
  ImGui::Text("Parent ID: %i"_j, (int)bone.getBoneParent());
};

kpi::DecentralizedInstaller AddBoneTransformSurface([](kpi::ApplicationPlugins&) {
  auto& inst = kpi::PropertyViewManager::getInstance();
  inst.addPropertyView<libcube::IBoneDelegate, BoneTransformSurface>();
});

} // namespace libcube::UI
