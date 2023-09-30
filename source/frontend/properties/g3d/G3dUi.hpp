#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/gc/Export/Material.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d::ui {

struct G3DDataSurface final {
  static inline const char* name() { return "Fog"_j; }
  static inline const char* icon = (const char*)ICON_FA_SHIP;
  static inline ImVec4 color = Clr(0x588157);
};
void drawProperty(kpi::PropertyDelegate<Material>& delegate, G3DDataSurface);

struct G3DTexDataSurface final {
  static inline const char* name() { return "BRRES Data"_j; }
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};
void drawProperty(kpi::PropertyDelegate<Texture>& delegate, G3DTexDataSurface);

struct G3DMdlDataSurface final {
  static inline const char* name() { return "BRRES Data"_j; }
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};
void drawProperty(kpi::PropertyDelegate<Model>& delegate, G3DMdlDataSurface);

} // namespace riistudio::g3d::ui
