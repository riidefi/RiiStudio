#include "ResizeAction.hpp"
#include <core/3d/i3dmodel.hpp>
#include <core/kpi/ActionMenu.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/util/gui.hpp>
#include <frontend/widgets/Image.hpp>
#include <frontend/widgets/Lib3dImage.hpp>
#include <librii/image/TextureExport.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Texture.hpp>
#include <rsl/FsDialog.hpp>
#include <vendor/stb_image.h>

import std.core;

namespace libcube::UI {

struct PopupOpener {
  ~PopupOpener() {
    if (name != nullptr)
      ImGui::OpenPopup(name);
  }

  void open(const char* _name) { name = _name; }

  const char* name = nullptr;
};

static const std::vector<std::string> StdImageFilters = {
    "PNG Files", "*.png",     "TGA Files", "*.tga",     "JPG Files",
    "*.jpg",     "BMP Files", "*.bmp",     "All Files", "*",
};

void exportImage(const Texture& tex, u32 export_lod) {
  std::string path =
      tex.getName() + " Mip Level " + std::to_string(export_lod) + ".png";
  librii::STBImage imgType = librii::STBImage::PNG;

  if (rsl::FileDialogsSupported()) {
    auto results = rsl::SaveOneFile("Export image"_j, "", StdImageFilters);
    if (!results)
      return;
    path = results->string();
    if (path.ends_with(".png")) {
      imgType = librii::STBImage::PNG;
    } else if (path.ends_with(".bmp")) {
      imgType = librii::STBImage::BMP;
    } else if (path.ends_with(".tga")) {
      imgType = librii::STBImage::TGA;
    } else if (path.ends_with(".jpg")) {
      imgType = librii::STBImage::JPG;
    }
  }

  std::vector<u8> data;
  tex.decode(data, true);

  u32 offset = 0;
  for (u32 i = 0; i < export_lod; ++i)
    offset += (tex.getWidth() >> i) * (tex.getHeight() >> i) * 4;

  librii::writeImageStbRGBA(path.c_str(), imgType, tex.getWidth() >> export_lod,
                            tex.getHeight() >> export_lod,
                            data.data() + offset);
}

void importImage(Texture& tex, u32 import_lod) {
  auto path = rsl::OpenOneFile("Import image"_j, "", StdImageFilters);
  if (!path) {
    return;
  }
  int width, height, channels;
  unsigned char* image = stbi_load(path->string().c_str(), &width, &height,
                                   &channels, STBI_rgb_alpha);
  assert(image);
  const auto fmt = tex.getTextureFormat();
  const auto offset =
      import_lod == 0
          ? 0
          : librii::image::getEncodedSize(tex.getWidth(), tex.getHeight(), fmt,
                                          import_lod - 1);
  librii::image::transform(tex.getData() + offset, tex.getWidth() >> import_lod,
                           tex.getHeight() >> import_lod,
                           gx::TextureFormat::Extension_RawRGBA32, fmt, image,
                           width, height);
}

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
    if (ImGui::MenuItem("Resize"_j)) {
      resize_reset();
      resize = true;
    }
    if (ImGui::MenuItem("Reformat"_j)) {
      reformat_reset();
      reformat = true;
    }

    if (ImGui::BeginMenu("Export"_j)) {
      if (lastTex != &tex) {
        mImg.clear();
        mImg.resize(tex.getMipmapCount() + 1);
        for (int i = 0; i <= tex.getMipmapCount(); ++i)
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

    if (ImGui::BeginMenu("Import"_j)) {
      if (lastTex != &tex) {
        mImg.clear();
        mImg.resize(tex.getMipmapCount() + 1);
        for (int i = 0; i <= tex.getMipmapCount(); ++i)
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
      importImage(tex, import_lod);
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

void drawProperty(kpi::PropertyDelegate<Texture>& delegate, ImageSurface& tex) {
  auto& data = delegate.getActive();

  tex.attached = &data;

  bool resizeAction = false;
  bool reformatOption = false;

  if (ImGui::BeginMenuBar()) {
    // if (ImGui::BeginMenu("Transform")) {
    if (ImGui::Button((const char*)ICON_FA_DRAW_POLYGON u8" Resize")) {
      resizeAction = true;
    }
    if (ImGui::Button((const char*)ICON_FA_DRAW_POLYGON u8" Change format")) {
      reformatOption = true;
    }
    if (ImGui::Button((const char*)ICON_FA_SAVE u8" Export")) {
      exportImage(data, 0);
    }
    if (ImGui::Button((const char*)ICON_FA_FILE u8" Import")) {
      auto path = rsl::OpenOneFile("Import image", "", StdImageFilters);
      if (path) {
        int width, height, channels;
        unsigned char* image = stbi_load(path->string().c_str(), &width,
                                         &height, &channels, STBI_rgb_alpha);
        assert(image);
        data.setWidth(width);
        data.setHeight(height);
        data.setMipmapCount(0);
        data.resizeData();
        data.encode(image);
        stbi_image_free(image);
        tex.invalidateTextureCaches();
        delegate.commit("Import Image");
      }
    }
    // ImGui::EndMenu();
    // }
    ImGui::EndMenuBar();
  }

  ImGui::Text(
      (const char*)
          ICON_FA_EXCLAMATION_TRIANGLE u8" Image Properties do not support "
                                       u8"multi-selection currently.");

  if (resizeAction) {
    ImGui::OpenPopup("Resize");
    tex.resize_reset();
  } else if (reformatOption) {
    ImGui::OpenPopup("Reformat");
    tex.reformat_reset();
  }

  if (ImGui::BeginPopupModal("Reformat", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    bool changed = false;
    const bool keep_alive = tex.reformat_draw(data, &changed);

    if (changed) {
      tex.invalidateTextureCaches();
      delegate.commit("Reformat Image");
    }

    if (!keep_alive)
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
  if (ImGui::BeginPopupModal("Resize", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    bool changed = false;
    const bool keep_alive = tex.resize_draw(data, &changed);

    if (changed) {
      tex.invalidateTextureCaches();
      delegate.commit("Resize Image");
    }

    if (!keep_alive)
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }

  tex.mImg.draw(data);

  const auto name = data.getName();
  tex.nameBuf.clear();
  tex.nameBuf.resize(name.size() * 2 + 1 /* null terminator */);
  tex.nameBuf.insert(tex.nameBuf.begin(), name.begin(), name.end());
  ImGui::InputText("Name", tex.nameBuf.data(), tex.nameBuf.size());
  if (strcmp(tex.nameBuf.data(), name.data())) {
    data.setName(tex.nameBuf.data());
    data.onUpdate();
    delegate.commit("Rename");
  }

#ifdef BUILD_DEBUG
  if (ImGui::CollapsingHeader("DEBUG")) {
    int width = data.getWidth();
    ImGui::InputInt("width", &width);
    data.setWidth(width);
    int mmCnt = data.getMipmapCount();
    ImGui::InputInt("Mipmap Count", &mmCnt);
    data.setMipmapCount(mmCnt);
  }
#endif
}

kpi::RegisterPropertyView<libcube::Texture, ImageSurface> ImageSurfaceInstaller;

kpi::DecentralizedInstaller
    ImageActionsInstaller([](kpi::ApplicationPlugins& installer) {
      kpi::ActionMenuManager::get().addMenu(std::make_unique<ImageActions>());
    });

} // namespace libcube::UI
