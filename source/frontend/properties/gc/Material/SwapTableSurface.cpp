#include "Common.hpp"
#include <imcxx/Widgets.hpp>

namespace libcube::UI {

struct SwapTableSurface final {
  static inline const char* name() { return "Swizzling"_j; }
  static inline const char* icon = (const char*)ICON_FA_SWATCHBOOK;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SwapTableSurface) {
  auto& matData = delegate.getActive().getMaterialData();

  const char* colors = "Red\0"
                       "Green\0"
                       "Blue\0"
                       "Alpha\0"_j;
  ImGui::BeginTable("Swap Tables"_j, 5, ImGuiTableFlags_Borders);
  ImGui::TableSetupColumn("Table ID"_j);
  ImGui::TableSetupColumn("Red Destination"_j);
  ImGui::TableSetupColumn("Green Destination"_j);
  ImGui::TableSetupColumn("Blue Destination"_j);
  ImGui::TableSetupColumn("Alpha Destination"_j);
  ImGui::TableHeadersRow();
  ImGui::TableNextRow();
  for (int i = 0; i < matData.mSwapTable.size(); ++i) {
    ImGui::PushID(i);
    auto& swap = matData.mSwapTable[i];

    ImGui::TableNextColumn();
    ImGui::Text("Swap %i"_j, i);
    ImGui::TableNextColumn();

	ImGui::PushItemWidth(ImGui::GetColumnWidth());
    auto r = imcxx::Combo("##R", swap.r, colors);
    ImGui::PopItemWidth();
    ImGui::TableNextColumn();
    ImGui::PushItemWidth(ImGui::GetColumnWidth());
    auto g = imcxx::Combo("##G", swap.g, colors);
    ImGui::PopItemWidth();
    ImGui::TableNextColumn();
    ImGui::PushItemWidth(ImGui::GetColumnWidth());
    auto b = imcxx::Combo("##B", swap.b, colors);
    ImGui::PopItemWidth();
    ImGui::TableNextColumn();
    ImGui::PushItemWidth(ImGui::GetColumnWidth());
    auto a = imcxx::Combo("##A", swap.a, colors);
    ImGui::PopItemWidth();
    ImGui::TableNextColumn();

    AUTO_PROP(mSwapTable[i].r, r);
    AUTO_PROP(mSwapTable[i].g, g);
    AUTO_PROP(mSwapTable[i].b, b);
    AUTO_PROP(mSwapTable[i].a, a);

    ImGui::PopID();
    ImGui::TableNextRow();
  }

  ImGui::EndTable();
}

kpi::RegisterPropertyView<IGCMaterial, SwapTableSurface> SwapTableInstaller;

} // namespace libcube::UI
