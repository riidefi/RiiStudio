#include "Common.hpp"

namespace libcube::UI {

struct CullMode {
  bool front, back;

  CullMode() : front(true), back(false) {}
  CullMode(bool f, bool b) : front(f), back(b) {}
  CullMode(librii::gx::CullMode c) { set(c); }

  void set(librii::gx::CullMode c) {
    switch (c) {
    case librii::gx::CullMode::All:
      front = back = false;
      break;
    case librii::gx::CullMode::None:
      front = back = true;
      break;
    case librii::gx::CullMode::Front:
      front = false;
      back = true;
      break;
    case librii::gx::CullMode::Back:
      front = true;
      back = false;
      break;
    default:
      assert(!"Invalid cull mode");
      front = back = true;
      break;
    }
  }
  librii::gx::CullMode get() const noexcept {
    if (front && back)
      return librii::gx::CullMode::None;

    if (!front && !back)
      return librii::gx::CullMode::All;

    return front ? librii::gx::CullMode::Back : librii::gx::CullMode::Front;
  }

  void draw() {
    ImGui::Text("Show sides of faces:");
    ImGui::Checkbox("Front", &front);
    ImGui::Checkbox("Back", &back);
  }
};
librii::gx::CullMode DrawCullMode(librii::gx::CullMode cull_mode) {
  CullMode widget(cull_mode);
  widget.draw();
  return widget.get();
}

struct DisplaySurface final {
  static inline const char* name = "Surface Visibility";
  static inline const char* icon = (const char*)ICON_FA_GHOST;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  DisplaySurface) {
  mat_prop_ex(delegate, &GCMaterialData::cullMode, DrawCullMode);
}

kpi::RegisterPropertyView<IGCMaterial, DisplaySurface> DisplaySurfaceInstaller;

} // namespace libcube::UI
