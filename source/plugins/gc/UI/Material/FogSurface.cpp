#include "Common.hpp"

namespace libcube::UI {

struct FogSurface final {
  static inline const char* name = "Fog";
  static inline const char* icon = (const char*)ICON_FA_GHOST;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate, FogSurface) {
  // auto& matData = delegate.getActive().getMaterialData();
}

} // namespace libcube::UI
