#include <core/kpi/Node2.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/util/gui.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

librii::gx::Color ColorEditU8(const char* name, librii::gx::Color clr) {
  auto fclr = static_cast<librii::gx::ColorF32>(clr);

  ImGui::ColorEdit4(name, fclr);
  fclr.clamp(0.0f, 1.0f);

  return fclr;
}

Quantization DrawColorQuantization(Quantization q) {
  auto type = imcxx::Combo("Format", q.mType.color,
                           "FORMAT_16B_565  [RGB565]\0"
                           "FORMAT_24B_888  [RGB8]\0"
                           "FORMAT_32B_888x [RGBX8]\0"
                           "FORMAT_16B_4444 [RGBA4]\0"
                           "FORMAT_24B_6666 [RGBA6]\0"
                           "FORMAT_32B_8888 [RGBA8]\0");

  if (type == q.mType.color)
    return q;

  Quantization result = q;

  result.mType.color = type;

  using C = librii::gx::VertexBufferType::Color;
  using F = librii::gx::VertexComponentCount::Color;

  switch (type) {
  case C::rgb565:
    result.mComp.color = F::rgb;
    result.stride = 2;
    break;
  case C::rgb8:
    result.mComp.color = F::rgb;
    result.stride = 3;
    break;
  case C::rgbx8:
    result.mComp.color = F::rgb;
    result.stride = 4;
    break;
  case C::rgba4:
    result.mComp.color = F::rgba;
    result.stride = 2;
    break;
  case C::rgba6:
    result.mComp.color = F::rgba;
    result.stride = 3;
    break;
  case C::rgba8:
    result.mComp.color = F::rgba;
    result.stride = 4;
    break;
  }

  return result;
}

struct G3dVertexColorDataSurface {
  static inline const char* name = "Data";
  static inline const char* icon = (const char*)ICON_FA_PAINT_BRUSH;
};
struct G3dVertexColorQuantSurface {
  static inline const char* name = "Format";
  static inline const char* icon = (const char*)ICON_FA_COG;
};

void drawProperty(kpi::PropertyDelegate<ColorBuffer>& dl,
                  G3dVertexColorQuantSurface) {
  auto& clr_group = dl.getActive();

  auto edited = DrawColorQuantization(clr_group.mQuantize);

  KPI_PROPERTY_EX(dl, mQuantize.mType.color, edited.mType.color);
  KPI_PROPERTY_EX(dl, mQuantize.mComp.color, edited.mComp.color);
  KPI_PROPERTY_EX(dl, mQuantize.stride, edited.stride);
}

void drawProperty(kpi::PropertyDelegate<ColorBuffer>& dl,
                  G3dVertexColorDataSurface) {
  auto& clr_group = dl.getActive();

  // if (ImGui::BeginTable("Vertex Colors", 2)) {
  int i = 0;
  for (auto& clr : clr_group.mEntries) {
    auto new_color = ColorEditU8(std::to_string(i).c_str(), clr);
    KPI_PROPERTY(dl, clr, new_color, mEntries[i]);
    ++i;
    //      ImGui::TableNextRow();
  }

  //    ImGui::EndTable();
  // }
} // namespace riistudio::g3d

kpi::DecentralizedInstaller VColorInstaller([](kpi::ApplicationPlugins&) {
  auto& inst = kpi::PropertyViewManager::getInstance();
  inst.addPropertyView<ColorBuffer, G3dVertexColorDataSurface>();
  inst.addPropertyView<ColorBuffer, G3dVertexColorQuantSurface>();
});

} // namespace riistudio::g3d
