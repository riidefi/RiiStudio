#include "Common.hpp"
#include <core/kpi/PropertyView.hpp>
#include <core/util/gui.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>

namespace libcube::UI {

auto PolyDescriptorSurface =
    kpi::StatelessPropertyView<libcube::IndexedPolygon>()
        .setTitle("Vertex Descriptor")
        .setIcon((const char*)ICON_FA_IMAGE)
        .onDraw([](kpi::PropertyDelegate<IndexedPolygon>& dl) {
          auto& poly = dl.getActive();

          auto& desc = poly.getVcd();

          int i = 0;
          for (auto& attrib : desc.mAttributes) {
            riistudio::util::IDScope g(i++);

            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() / 3);

            int type = static_cast<int>(attrib.first);
            ImGui::Combo("Attribute Type", &type, vertexAttribNames);
            int format = static_cast<int>(attrib.second);
            ImGui::SameLine();
            ImGui::Combo(
                "Attribute Format", &format,
                "None\0Direct\0U8 / 8-bit / 0-255\0U16 / 16-bit / 0-65535\0");

            ImGui::PopItemWidth();
          }
        });

} // namespace libcube::UI
