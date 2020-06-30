#define STB_IMAGE_IMPLEMENTATION
#include <vendor/stb_image.h>

#include <vendor/FileDialogues.hpp>

#include <algorithm>
#include <core/3d/i3dmodel.hpp>
#include <core/util/gui.hpp>
#include <core/3d/ui/Image.hpp>
#include <core/kpi/PropertyView.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Texture.hpp>

#include <plugins/gc/Util/TextureExport.hpp>

namespace libcube::UI {

struct ImageSurface final {
  static inline const char* name = "Image Data";
  static inline const char* icon = (const char*)ICON_FA_IMAGE;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  struct ResizeDimension {
    const char* name = "?";
    int before = -1;
    int value = -1;
    bool constrained = true;
  };

  std::array<ResizeDimension, 2> resize{
      ResizeDimension{"Width ", -1, -1, false},
      ResizeDimension{"Height", -1, -1, true}};
  int reformatOpt = -1;
  int resizealgo = 0;
  Texture* lastTex = nullptr;
  riistudio::frontend::ImagePreview mImg;
};

static const std::vector<std::string> StdImageFilters = {
    "PNG Files", "*.png",     "TGA Files", "*.tga",     "JPG Files",
    "*.jpg",     "BMP Files", "*.bmp",     "All Files", "*",
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
      auto results = pfd::save_file("Export image", "", StdImageFilters);
      if (!results.result().empty()) {
        const std::string path = results.result();
        libcube::STBImage imgType = libcube::STBImage::PNG;
        if (riistudio::util::ends_with(path, ".png")) {
          imgType = libcube::STBImage::PNG;
        } else if (riistudio::util::ends_with(path, ".bmp")) {
          imgType = libcube::STBImage::BMP;
        } else if (riistudio::util::ends_with(path, ".tga")) {
          imgType = libcube::STBImage::TGA;
        } else if (riistudio::util::ends_with(path, ".jpg")) {
          imgType = libcube::STBImage::JPG;
        }

        // Only top LOD
        libcube::writeImageStbRGBA(path.c_str(), imgType, data.getWidth(),
                                   data.getHeight(),
                                   tex.mImg.mDecodeBuf.data());
      }
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

  ImGui::Text((const char*)ICON_FA_EXCLAMATION_TRIANGLE
              u8" Image Properties do not support multi-selection currently.");

  if (resizeAction) {
    ImGui::OpenPopup("Resize");
    tex.resize[0].value = -1;
    tex.resize[1].value = -1;
    tex.resize[0].before = -1;
    tex.resize[1].before = -1;
  } else if (reformatOption) {
    ImGui::OpenPopup("Reformat");
    tex.reformatOpt = -1;
  }

#ifndef BUILD_DIST
  if (ImGui::BeginPopupModal("Reformat", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (tex.reformatOpt == -1) {
      tex.reformatOpt = data.getTextureFormat();
    }
    ImGui::InputInt("Format", &tex.reformatOpt);
    if (ImGui::Button((const char*)ICON_FA_CHECK u8" Okay")) {
      const auto oldFormat = data.getTextureFormat();
      data.setTextureFormat(tex.reformatOpt);
      data.resizeData();

      libcube::image_platform::transform(
          data.getData(), data.getWidth(), data.getHeight(),
          static_cast<libcube::gx::TextureFormat>(oldFormat),
          static_cast<libcube::gx::TextureFormat>(tex.reformatOpt),
          data.getData(), data.getWidth(), data.getHeight(),
          data.getMipmapCount() - 1);
      delegate.commit("Reformat Image");
      tex.lastTex = nullptr;

      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();

    if (ImGui::Button((const char*)ICON_FA_CROSS u8" Cancel")) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
#endif
  if (ImGui::BeginPopupModal("Resize", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (tex.resize[0].before <= 0) {
      tex.resize[0].before = data.getWidth();
    }
    if (tex.resize[1].before <= 0) {
      tex.resize[1].before = data.getHeight();
    }

    int dX = 0;
    for (auto& it : tex.resize) {
      auto& other = dX++ ? tex.resize[0] : tex.resize[1];

      int before = it.value;
      ImGui::InputInt(it.name, &it.value, 1, 64);
      ImGui::SameLine();
      ImGui::Checkbox((std::string("Constrained##") + it.name).c_str(),
                      &it.constrained);
      if (it.constrained) {
        other.constrained = false;
      }
      ImGui::SameLine();
      if (ImGui::Button((std::string("Reset##") + it.name).c_str())) {
        it.value = -1;
      }

      if (before != it.value) {
        if (it.constrained) {
          it.value = before;
        } else if (other.constrained) {
          other.value = other.before * it.value / it.before;
        }
      }
    }

    if (tex.resize[0].value <= 0) {
      tex.resize[0].value = data.getWidth();
    }
    if (tex.resize[1].value <= 0) {
      tex.resize[1].value = data.getHeight();
    }

    ImGui::Combo("Algorithm", (int*)&tex.resizealgo, "Ultimate\0Lanczos\0");

    if (ImGui::Button((const char*)ICON_FA_CHECK u8" Resize")) {
      printf("Do the resizing..\n");

      const auto oldWidth = data.getWidth();
      const auto oldHeight = data.getHeight();

      data.setWidth(tex.resize[0].value);
      data.setHeight(tex.resize[1].value);
      data.resizeData();

      libcube::image_platform::transform(
          data.getData(), tex.resize[0].value, tex.resize[1].value,
          static_cast<libcube::gx::TextureFormat>(data.getTextureFormat()),
          std::nullopt, data.getData(), oldWidth, oldHeight,
          data.getMipmapCount(),
          static_cast<libcube::image_platform::ResizingAlgorithm>(
              tex.resizealgo));
      delegate.commit("Resize Image");
      tex.lastTex = nullptr;

      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();

    if (ImGui::Button((const char*)ICON_FA_CROSS u8" Cancel")) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  if (tex.lastTex != &data) {
    tex.lastTex = &data;
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

} // namespace libcube::UI
