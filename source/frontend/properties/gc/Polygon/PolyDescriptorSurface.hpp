#include <LibBadUIFramework/PropertyView.hpp>
#include <frontend/properties/gc/Polygon/Common.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>

namespace libcube::UI {

struct PolyDescriptorSurface final {
  static inline const char* name() { return "Vertex Descriptor"_j; }
  static inline const char* icon = (const char*)ICON_FA_IMAGE;
};
void drawProperty(kpi::PropertyDelegate<libcube::IndexedPolygon>& delegate,
                  PolyDescriptorSurface);

} // namespace libcube::UI
