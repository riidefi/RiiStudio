#pragma once

#include "ResizeAction.hpp"
#include <LibBadUIFramework/ActionMenu.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <core/3d/i3dmodel.hpp>
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

[[nodiscard]] Result<void> importImage(Texture& tex, u32 import_lod);
void exportImage(const Texture& tex, u32 export_lod);

class ImageActions : public kpi::ActionMenu<Texture, ImageActions>,
                     public ResizeAction,
                     public ReformatAction {
  bool resize = false, reformat = false;

  int export_lod = -1;
  int import_lod = -1;

  std::vector<riistudio::frontend::ImagePreview> mImg;
  Texture* lastTex = nullptr;

  // Return true if the state of obj was mutated (add to history)
public:
  bool _context(Texture& tex) {
    if (ImGui::MenuItem("Resize (Keeping custom mipmaps)"_j)) {
      resize_reset();
      resize = true;
    }
    if (ImGui::MenuItem("Reformat (Keeping custom mipmaps)"_j)) {
      reformat_reset();
      reformat = true;
    }

    if (ImGui::BeginMenu("Export Mipmap"_j)) {
      if (lastTex != &tex) {
        mImg.clear();
        mImg.resize(tex.getMipmapCount() + 1);
        for (u32 i = 0; i <= tex.getMipmapCount(); ++i)
          mImg[i].setFromImage(tex);
        lastTex = &tex;
      }

      export_lod = -1;

      mImg[0].draw(75, 75, false);
      if (ImGui::MenuItem("Base Image (LOD 0)"_j)) {
        export_lod = 0;
      }
      if (tex.getMipmapCount() > 0) {
        ImGui::Separator();
        for (unsigned i = 1; i <= tex.getMipmapCount(); ++i) {
          mImg[i].mLod = i;
          mImg[i].draw(75, 75, false);
          if (ImGui::MenuItem(("LOD " + std::to_string(i)).c_str())) {
            export_lod = i;
          }
        }
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Import Mipmap"_j)) {
      if (lastTex != &tex) {
        mImg.clear();
        mImg.resize(tex.getMipmapCount() + 1);
        for (u32 i = 0; i <= tex.getMipmapCount(); ++i)
          mImg[i].setFromImage(tex);
        lastTex = &tex;
      }

      import_lod = -1;

      mImg[0].draw(75, 75, false);
      if (ImGui::MenuItem("Base Image (LOD 0)"_j)) {
        import_lod = 0;
      }
      if (tex.getMipmapCount() > 0) {
        ImGui::Separator();
        for (unsigned i = 1; i <= tex.getMipmapCount(); ++i) {
          mImg[i].mLod = i;
          mImg[i].draw(75, 75, false);
          if (ImGui::MenuItem(("LOD " + std::to_string(i)).c_str())) {
            import_lod = i;
          }
        }
      }

      ImGui::EndMenu();
    }

    return false;
  }

  bool _modal(Texture& tex) {
    bool all_changed = false;

    if (export_lod != -1) {
      exportImage(tex, export_lod);
      export_lod = -1;
    }

    if (import_lod != -1) {
      auto ok = importImage(tex, import_lod);
      if (!ok) {
        rsl::ErrorDialogFmt("Failed to import image: {}", ok.error());
      }
      all_changed = true;
      import_lod = -1;
      if (lastTex != nullptr)
        lastTex->onUpdate();
      lastTex = nullptr;
    }

    if (resize) {
      ImGui::OpenPopup("Resize");
      resize = false;
    }
    if (reformat) {
      ImGui::OpenPopup("Reformat");
      reformat = false;
    }

    if (ImGui::BeginPopupModal("Resize", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      bool changed = false;
      const bool keep_alive = resize_draw(tex, &changed);
      all_changed |= changed;

      if (!keep_alive)
        ImGui::CloseCurrentPopup();

      ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Reformat", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      bool changed = false;
      const bool keep_alive = reformat_draw(tex, &changed);
      all_changed |= changed;

      if (!keep_alive)
        ImGui::CloseCurrentPopup();

      ImGui::EndPopup();
    }

    return all_changed;
  }
};

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

void ImageActionsInstaller();

} // namespace libcube::UI
