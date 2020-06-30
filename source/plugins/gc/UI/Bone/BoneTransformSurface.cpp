#include <core/util/gui.hpp>
#include <core/kpi/PropertyView.hpp>
#include <plugins/gc/Export/Bone.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace libcube::UI {

auto bone_transform_ui = [](kpi::PropertyDelegate<IBoneDelegate>& delegate) {
  auto& bone = delegate.getActive();

  const auto srt = bone.getSRT();
  glm::vec3 scl = srt.scale;
  glm::vec3 rot = srt.rotation;
  glm::vec3 pos = srt.translation;

  ImGui::InputFloat3("Scale", &scl.x);
  delegate.property(
      bone.getScale(), scl, [](const auto& x) { return x.getScale(); },
      [](auto& x, const auto& y) { x.setScale(y); });

  ImGui::InputFloat3("Rotation", &rot.x);
  delegate.property(
      bone.getRotation(), rot, [](const auto& x) { return x.getRotation(); },
      [](auto& x, const auto& y) { x.setRotation(y); });
  ImGui::InputFloat3("Translation", &pos.x);
  delegate.property(
      bone.getTranslation(), pos,
      [](const auto& x) { return x.getTranslation(); },
      [](auto& x, const auto& y) { x.setTranslation(y); });
  ImGui::Text("Parent ID: %i", (int)bone.getBoneParent());
};

kpi::StatelessPropertyView<libcube::IBoneDelegate>
    BoneTransformSurfaceInstaller("Transformation",
                                  (const char*)ICON_FA_ARROWS_ALT,
                                  bone_transform_ui);

} // namespace libcube::UI
