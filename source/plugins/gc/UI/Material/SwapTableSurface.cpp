#include "Common.hpp"

namespace libcube::UI {

struct SwapTableSurface final {
  static inline const char* name = "Swap Tables";
  static inline const char* icon = (const char*)ICON_FA_SWATCHBOOK;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SwapTableSurface) {
  auto& matData = delegate.getActive().getMaterialData();

  const char* colors = "Sample from Red\0Sample from Green\0Sample from "
                       "Blue\0Sample from Alpha\0";
  ImGui::BeginTable("Swap Tables", 5, ImGuiTableFlags_Borders);
  ImGui::TableSetupColumn("Table ID");
  ImGui::TableSetupColumn("Red Destination");
  ImGui::TableSetupColumn("Green Destination");
  ImGui::TableSetupColumn("Blue Destination");
  ImGui::TableSetupColumn("Alpha Destination");
  ImGui::TableAutoHeaders();
  ImGui::TableNextRow();
  for (int i = 0; i < matData.shader.mSwapTable.size(); ++i) {
    ImGui::PushID(i);
    auto& swap = matData.shader.mSwapTable[i];

    int r = static_cast<int>(swap.r);
    int g = static_cast<int>(swap.g);
    int b = static_cast<int>(swap.b);
    int a = static_cast<int>(swap.a);

    ImGui::Text("Swap %i", i);
    ImGui::TableNextCell();

    ImGui::Combo("##R", &r, colors);
    ImGui::TableNextCell();
    ImGui::Combo("##G", &g, colors);
    ImGui::TableNextCell();
    ImGui::Combo("##B", &b, colors);
    ImGui::TableNextCell();
    ImGui::Combo("##A", &a, colors);
    ImGui::TableNextCell();

    AUTO_PROP(shader.mSwapTable[i].r, static_cast<gx::ColorComponent>(r));
    AUTO_PROP(shader.mSwapTable[i].g, static_cast<gx::ColorComponent>(g));
    AUTO_PROP(shader.mSwapTable[i].b, static_cast<gx::ColorComponent>(b));
    AUTO_PROP(shader.mSwapTable[i].a, static_cast<gx::ColorComponent>(a));

    ImGui::PopID();
    ImGui::TableNextRow();
  }

  ImGui::EndTable();
}

kpi::RegisterPropertyView<IGCMaterial, SwapTableSurface> SwapTableInstaller;

} // namespace libcube::UI
