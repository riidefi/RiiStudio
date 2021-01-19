#include <core/kpi/PropertyView.hpp>
#include <core/util/gui.hpp>
#include <plugins/gc/Export/Bone.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Material.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace libcube::UI {

auto BoneDisplaySurface =
    kpi::StatelessPropertyView<libcube::IBoneDelegate>()
        .setTitle("Displays")
        .setIcon((const char*)ICON_FA_IMAGE)
        .onDraw([](kpi::PropertyDelegate<IBoneDelegate>& delegate) {
          auto& bone = delegate.getActive();
          ImGui::Text(
              (const char*)
                  ICON_FA_EXCLAMATION_TRIANGLE u8" Display Properties do not "
                                               u8"currently support "
                                               u8"multi-selection.");

          auto folder_id_combo = [](const char* title, const auto& folder,
                                    int& active) {
            ImGui::PushItemWidth(200);
            if (ImGui::BeginCombo(title, folder[active].getName().c_str())) {
              int j = 0;
              for (const auto& node : folder) {
                if (ImGui::Selectable(node.getName().c_str(), active == j)) {
                  active = j;
                }

                if (active == j)
                  ImGui::SetItemDefaultFocus();

                ++j;
              }

              ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
          };

          const auto entry_flags =
              ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable |
              ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
          if (ImGui::BeginTable("Entries", 3, entry_flags)) {
            const riistudio::lib3d::Model* pMdl =
                dynamic_cast<const riistudio::lib3d::Model*>(
                    dynamic_cast<const kpi::IObject*>(&bone)->childOf);
            const auto materials = pMdl->getMaterials();
            const auto polys = pMdl->getMeshes();

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
              folder_id_combo("Material", materials, matId);
              display.matId = matId;

              ImGui::TableSetColumnIndex(2);
              int polyId = display.polyId;
              folder_id_combo("Polygon", polys, polyId);
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
        });

} // namespace libcube::UI
