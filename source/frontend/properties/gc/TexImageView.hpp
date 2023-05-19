#pragma once

#include "ResizeAction.hpp"
#include <core/3d/i3dmodel.hpp>
#include <LibBadUIFramework/ActionMenu.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <frontend/widgets/Image.hpp>
#include <frontend/widgets/Lib3dImage.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/image/TextureExport.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Texture.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/Stb.hpp>
#include <vendor/stb_image.h>

namespace libcube::UI {

struct ImageSurface final : public ResizeAction, public ReformatAction {
  static inline const char* name() { return "Image Data"; }
  static inline const char* icon = (const char*)ICON_FA_IMAGE;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  Texture* attached = nullptr;
  riistudio::frontend::Lib3dCachedImagePreview mImg;
  std::vector<char> nameBuf;

  void invalidateTextureCaches() {
    if (attached != nullptr)
      attached->nextGenerationId();
  }
};

void drawProperty(kpi::PropertyDelegate<Texture>& delegate, ImageSurface& tex);

} // namespace libcube::UI
