#include <vendor/FileDialogues.hpp>
#include <vendor/stb_image.h>

#include <algorithm>

#include <core/3d/i3dmodel.hpp>
#include <core/3d/ui/Image.hpp>
#include <core/kpi/ActionMenu.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/util/gui.hpp>

#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Texture.hpp>
#include <plugins/gc/Util/TextureExport.hpp>

#include "ResizeAction.hpp"

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
  libcube::STBImage imgType = libcube::STBImage::PNG;

#ifdef RII_PLATFORM_WINDOWS
  auto results = pfd::save_file("Export image", "", StdImageFilters);
  if (results.result().empty())
    return;
  path = results.result();
  if (path.ends_with(".png")) {
    imgType = libcube::STBImage::PNG;
  } else if (path.ends_with(".bmp")) {
    imgType = libcube::STBImage::BMP;
  } else if (path.ends_with(".tga")) {
    imgType = libcube::STBImage::TGA;
  } else if (path.ends_with(".jpg")) {
    imgType = libcube::STBImage::JPG;
  }
#endif

  std::vector<u8> data;
  tex.decode(data, true);

  u32 offset = 0;
  for (u32 i = 0; i < export_lod; ++i)
    offset += (tex.getWidth() >> i) * (tex.getHeight() >> i) * 4;

  libcube::writeImageStbRGBA(
      path.c_str(), imgType, tex.getWidth() >> export_lod,
      tex.getHeight() >> export_lod, data.data() + offset);
}

void importImage(Texture& tex, u32 import_lod) {
  auto result = pfd::open_file("Import image", "", StdImageFilters).result();
  if (!result.empty()) {
    const auto path = result[0];
    int width, height, channels;
    unsigned char* image =
        stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    assert(image);
    const auto fmt = static_cast<gx::TextureFormat>(tex.getTextureFormat());
    const auto offset =
        import_lod == 0
            ? 0
            : libcube::image_platform::getEncodedSize(
                  tex.getWidth(), tex.getHeight(), fmt, import_lod - 1);
    libcube::image_platform::transform(
        tex.getData() + offset, tex.getWidth() >> import_lod,
        tex.getHeight() >> import_lod, gx::TextureFormat::Extension_RawRGBA32,
        fmt, image, width, height);
  }
}

class ImageActions : public kpi::ActionMenu<Texture, ImageActions>,
                     public ResizeAction,
                     public ReformatAction {
  bool resize = false, reformat = false;

  int export_lod = -1;
  int import_lod = -1;

  std::vector<riistudio::frontend::ImagePreview> mImg;
  const Texture* lastTex = nullptr;

  // Return true if the state of obj was mutated (add to history)
public:
  bool _context(Texture& tex) {
    if (ImGui::MenuItem("Resize")) {
      resize_reset();
      resize = true;
    }
    if (ImGui::MenuItem("Reformat")) {
      reformat_reset();
      reformat = true;
    }

    if (ImGui::BeginMenu("Export")) {
      if (lastTex != &tex) {
        mImg.clear();
        mImg.resize(tex.getMipmapCount() + 1);
        for (int i = 0; i <= tex.getMipmapCount(); ++i)
          mImg[i].setFromImage(tex);
        lastTex = &tex;
      }

      export_lod = -1;

      mImg[0].draw(75, 75, false);
      if (ImGui::MenuItem("Base Image (LOD 0)")) {
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

    if (ImGui::BeginMenu("Import")) {
      if (lastTex != &tex) {
        mImg.clear();
        mImg.resize(tex.getMipmapCount() + 1);
        for (int i = 0; i <= tex.getMipmapCount(); ++i)
          mImg[i].setFromImage(tex);
        lastTex = &tex;
      }

      import_lod = -1;

      mImg[0].draw(75, 75, false);
      if (ImGui::MenuItem("Base Image (LOD 0)")) {
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
        lastTex->notifyObservers();
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

struct ImageSurface final : public riistudio::lib3d::Texture::IObserver,
                            public ResizeAction,
                            public ReformatAction {
  static inline const char* name = "Image Data";
  static inline const char* icon = (const char*)ICON_FA_IMAGE;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  Texture* lastTex = nullptr;
  Texture* attached = nullptr;
  riistudio::frontend::ImagePreview mImg;

  void update(const riistudio::lib3d::Texture* tex) override {
    if (tex == lastTex)
      lastTex = nullptr;
  }

  ~ImageSurface() {
    if (attached != nullptr)
      attached->observers.erase(
          std::remove_if(attached->observers.begin(), attached->observers.end(),
                         [&](auto* x) { return x == this; }));
  }
};

void drawProperty(kpi::PropertyDelegate<Texture>& delegate, ImageSurface& tex) {
  auto& data = delegate.getActive();

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
      auto result =
          pfd::open_file("Import image", "", StdImageFilters).result();
      if (!result.empty()) {
        const auto path = result[0];
        int width, height, channels;
        unsigned char* image =
            stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        assert(image);
        data.setWidth(width);
        data.setHeight(height);
        data.setMipmapCount(0);
        data.resizeData();
        data.encode(image);
        stbi_image_free(image);
        delegate.commit("Import Image");
        tex.lastTex = nullptr;
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
      delegate.commit("Reformat Image");
      tex.lastTex = nullptr;
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
      delegate.commit("Resize Image");
      tex.lastTex = nullptr;
    }

    if (!keep_alive)
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }

  if (tex.lastTex != &data) {
    tex.lastTex = &data;
    if (tex.attached != nullptr)
      tex.attached->observers.erase(std::remove_if(
          tex.attached->observers.begin(), tex.attached->observers.end(),
          [&](auto* x) { return x == &tex; }));
    tex.attached = &data;
    data.observers.push_back(&tex);
    tex.mImg.setFromImage(data);
  }
  tex.mImg.draw();

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
