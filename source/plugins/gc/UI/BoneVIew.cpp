#include <vendor/fa5/IconsFontAwesome5.h>
#include <core/util/gui.hpp>
#include <algorithm>

#undef near

#include <kpi/PropertyView.hpp>

#include <core/3d/i3dmodel.hpp>
#include <plugins/gc/Export/Bone.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>

#include <plugins/gc/Export/Material.hpp>
// Tables
#include <imgui/imgui_internal.h>


namespace libcube::UI {


struct BoneTransformSurface final {
  const char* name = "Transformation";
  const char* icon = ICON_FA_ARROWS_ALT;
};

struct BoneDisplaySurface final {
  const char* name = "Displays";
  const char* icon = ICON_FA_IMAGE;
};

void drawProperty(kpi::PropertyDelegate<IBoneDelegate>& delegate,
                  BoneTransformSurface) {
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
}
void drawProperty(kpi::PropertyDelegate<IBoneDelegate>& delegate,
                  BoneDisplaySurface) {
  auto& bone = delegate.getActive();
  ImGui::Text(ICON_FA_EXCLAMATION_TRIANGLE
              " Display Properties do not currently support multi-selection.");

  auto folder_id_combo = [](const char* title, const auto& folder,
                            int& active) {
    if (ImGui::BeginCombo(title, folder[active]->getName().c_str())) {
      int j = 0;
      for (const auto& node : folder) {
        if (ImGui::Selectable(node->getName().c_str(), active == j)) {
          active = j;
        }

        if (active == j)
          ImGui::SetItemDefaultFocus();

        ++j;
      }

      ImGui::EndCombo();
    }
  };

  if (ImGui::BeginTable("Entries", 3, ImGuiTableFlags_Borders)) {
    const auto* materials = bone.getParent()->getFolder<libcube::IGCMaterial>();
    assert(materials);
    const auto* polys = bone.getParent()->getFolder<libcube::IndexedPolygon>();
    assert(polys);

    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

    ImGui::TableSetColumnIndex(0);
    ImGui::Text("ID");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("Material");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("Polygon");
    // ImGui::TableSetColumnIndex(3);
    // ImGui::Text("Sorting Priority");

    for (int i = 0; i < bone.getNumDisplays(); ++i) {
      ImGui::TableNextRow();

      ImGui::PushID(i);
      auto display = bone.getDisplay(i);

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%i", i);

      ImGui::TableSetColumnIndex(1);
      int matId = display.matId;
      folder_id_combo("Material", *materials, matId);
      display.matId = matId;

      ImGui::TableSetColumnIndex(2);
      int polyId = display.polyId;
      folder_id_combo("Polygon", *polys, polyId);
      display.polyId = polyId;

      // ImGui::TableSetColumnIndex(3);
      // int prio = display.prio;
      // ImGui::InputInt("Sorting Priority", &prio);
      // display.prio = prio;

      if (!(bone.getDisplay(i) == display)) {
        bone.setDisplay(i, display);
        delegate.commit("Edit Bone Display Entry");
      }

      ImGui::PopID();
    }

    ImGui::EndTable();
  }
}

void installBoneView() {
  kpi::PropertyViewManager& manager = kpi::PropertyViewManager::getInstance();

  manager.addPropertyView<libcube::IBoneDelegate, BoneTransformSurface>();
  manager.addPropertyView<libcube::IBoneDelegate, BoneDisplaySurface>();
}

} // namespace libcube::UI
