#include "Common.hpp"

namespace libcube::UI {

librii::gx::ColorS10 ColorEditS10(const char* name, librii::gx::ColorS10 clr) {
  auto fclr = static_cast<librii::gx::ColorF32>(clr);

  ImGui::ColorEdit4(name, fclr);
  fclr.clamp(-1024.0f / 255.0f, 1023.0f / 255.0f);

  return fclr;
}
librii::gx::Color ColorEditU8(const char* name, librii::gx::Color clr) {
  auto fclr = static_cast<librii::gx::ColorF32>(clr);

  ImGui::ColorEdit4(name, fclr);
  fclr.clamp(0.0f, 1.0f);

  return fclr;
}
struct ColorSurface final {
  static inline const char* name() { return "Colors"_j; }
  static inline const char* icon = (const char*)ICON_FA_PAINT_BRUSH;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate, ColorSurface) {
  auto& matData = delegate.getActive().getMaterialData();

  if (ImGui::CollapsingHeader("TEV Color Registers"_j,
                              ImGuiTreeNodeFlags_DefaultOpen)) {

    // TODO: Is CPREV default state accessible?
    for (std::size_t i = 0; i < 4; ++i) {
      auto color_name = std::string("Color Register "_j) + std::to_string(i);
      auto sclr = ColorEditS10(color_name.c_str(), matData.tevColors[i]);

      AUTO_PROP(tevColors[i], sclr);
    }
  }

  if (ImGui::CollapsingHeader("TEV Constant Colors"_j,
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    for (std::size_t i = 0; i < 4; ++i) {
      auto color_name = std::string("Constant Register "_j) + std::to_string(i);
      auto uclr = ColorEditU8(color_name.c_str(), matData.tevKonstColors[i]);

      AUTO_PROP(tevKonstColors[i], uclr);
    }
  }
}

kpi::RegisterPropertyView<IGCMaterial, ColorSurface> ColorSurfaceInstaller;

} // namespace libcube::UI
