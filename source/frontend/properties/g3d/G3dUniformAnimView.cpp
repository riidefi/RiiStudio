#include "G3dUniformAnimView.hpp"
#include <imcxx/Widgets.hpp>

namespace riistudio::g3d {

void drawProperty(kpi::PropertyDelegate<riistudio::g3d::CLR0>& dl,
                 G3dUniformAnimDataSurface) {
  auto& anim = dl.getActive();
  
  // Display the animation name
  if (ImGui::BeginTable("UniformAnimData", 2)) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Name");
    ImGui::TableNextColumn();
    ImGui::Text("%s", anim.name.c_str());
    ImGui::EndTable();
  }
}

} // namespace riistudio::g3d
