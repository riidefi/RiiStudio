#include <LibBadUIFramework/PropertyView.hpp>
#include <frontend/properties/gc/Polygon/Common.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <random>

namespace libcube::UI {

struct PolyDataSurface final {
  static inline const char* name() { return "Index Data"_j; }
  static inline const char* icon = (const char*)ICON_FA_IMAGE;
};
void drawProperty(kpi::PropertyDelegate<libcube::IndexedPolygon>& delegate,
                  PolyDataSurface);

} // namespace libcube::UI
