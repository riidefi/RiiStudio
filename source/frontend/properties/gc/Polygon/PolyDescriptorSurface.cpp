#include "PolyDescriptorSurface.hpp"

namespace libcube::UI {

void drawProperty(kpi::PropertyDelegate<libcube::IndexedPolygon>& dl,
	PolyDescriptorSurface) {
  auto& poly = dl.getActive();

  auto& desc = poly.getVcd();

  int i = 0;
  for (auto& attrib : desc.mAttributes) {
    riistudio::util::IDScope g(i++);

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 3);

    int type = static_cast<int>(attrib.first);
    ImGui::Combo("Attribute Type"_j, &type, vertexAttribNames);
    int format = static_cast<int>(attrib.second);
    ImGui::SameLine();
    ImGui::Combo("Attribute Format"_j, &format,
                 "None\0"
                 "Direct\0"
                 "U8 / 8-bit / 0-255\0"
                 "U16 / 16-bit / 0-65535\0"_j);

    ImGui::PopItemWidth();
  }
}

kpi::RegisterPropertyView<IndexedPolygon, PolyDescriptorSurface>
    PolyDescriptorSurface_;

} // namespace libcube::UI
