#include <algorithm>
#include <core/3d/i3dmodel.hpp>
#include <core/util/gui.hpp>
#include <kpi/PropertyView.hpp>

namespace libcube::UI {

struct ShaderSurface final {
  const char* name = "Pixel Shader";
  const char* icon = ICON_FA_IMAGE;
};

void drawProperty(kpi::PropertyDelegate<riistudio::lib3d::Material>& delegate,
                  ShaderSurface) {
  auto& mat = delegate.getActive();

  // Reset val (after applie to observers)
  mat.applyCacheAgain = false;

  if (ImGui::Button("Apply edited shader")) {
    mat.applyCacheAgain = true;
    mat.notifyObservers();
  }

  char buf[1024 * 12]{0};
  memcpy(buf, mat.cachedPixelShader.c_str(), mat.cachedPixelShader.size());
  ImGui::InputTextMultiline("Pixel Shader", buf, sizeof(buf), ImGui::GetContentRegionAvail());

  if (strcmp(buf, mat.cachedPixelShader.c_str()))
    mat.cachedPixelShader = buf;
}

kpi::RegisterPropertyView<riistudio::lib3d::Material, ShaderSurface>
    ShaderSurfaceInstaller;

} // namespace libcube::UI
