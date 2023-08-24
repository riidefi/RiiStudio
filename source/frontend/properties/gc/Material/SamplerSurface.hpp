#pragma once

#include <frontend/properties/gc/Material/Common.hpp>
#include <frontend/widgets/Lib3dImage.hpp> // for Lib3dCachedImagePreview
#include <imcxx/Widgets.hpp>
#include <librii/hx/TexGenType.hpp>
#include <librii/hx/TextureFilter.hpp>
#include <plugins/gc/Export/Scene.hpp>
#include <plugins/j3d/Material.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/SmallVector.hpp>

namespace libcube::UI {

struct SamplerSurface final {
  static inline const char* name() { return "Samplers"_j; }
  static inline const char* icon = (const char*)ICON_FA_IMAGES;
  static inline ImVec4 color = Clr(0x8AC926);

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::Lib3dCachedImagePreview mImg; // In mat sampler
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SamplerSurface& surface);

} // namespace libcube::UI
