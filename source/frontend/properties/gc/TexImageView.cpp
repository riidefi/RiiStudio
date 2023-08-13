#include "TexImageView.hpp"

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

[[nodiscard]] Result<void> exportImage(const Texture& tex, u32 export_lod) {
  std::string path =
      tex.getName() + " Mip Level " + std::to_string(export_lod) + ".png";
  librii::STBImage imgType = librii::STBImage::PNG;

  if (rsl::FileDialogsSupported()) {
    auto results = TRY(rsl::SaveOneFile("Export image"_j, "", StdImageFilters));
    path = results.string();
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

  return librii::writeImageStbRGBA(
      path.c_str(), imgType, tex.getWidth() >> export_lod,
      tex.getHeight() >> export_lod, data.data() + offset);
}

[[nodiscard]] Result<void> importImage(Texture& tex, u32 import_lod) {
  auto path = TRY(rsl::OpenOneFile("Import image"_j, "", StdImageFilters));
  auto image = TRY(rsl::stb::load(path.string()));
  const auto fmt = tex.getTextureFormat();
  const auto offset =
      import_lod == 0
          ? 0
          : librii::image::getEncodedSize(tex.getWidth(), tex.getHeight(), fmt,
                                          import_lod - 1);
  TRY(librii::image::transform(tex.getData().subspan(offset),
                               tex.getWidth() >> import_lod,
                               tex.getHeight() >> import_lod,
                               librii::gx::TextureFormat::Extension_RawRGBA32,
                               fmt, image.data, image.width, image.height));
  return {};
}

void drawProperty(kpi::PropertyDelegate<Texture>& delegate, ImageSurface& tex) {
  auto& data = delegate.getActive();

  tex.attached = &data;

  bool resizeAction = false;
  bool reformatOption = false;

  if (data.getWidth() > 1024 || data.getHeight() > 1024) {
    riistudio::util::PushErrorSyle();
    ImGui::TextUnformatted(
        (const char*)ICON_FA_EXCLAMATION_TRIANGLE u8"Error: Texture dimensions "
                                                  u8"exceed 1024x1024!");
    riistudio::util::PopErrorStyle();
  }

  if (ImGui::BeginMenuBar()) {
    // if (ImGui::BeginMenu("Transform")) {
    if (ImGui::Button((const char*)ICON_FA_DRAW_POLYGON u8" Resize")) {
      resizeAction = true;
    }
    if (ImGui::Button((const char*)ICON_FA_DRAW_POLYGON u8" Change format")) {
      reformatOption = true;
    }
    if (ImGui::Button((const char*)ICON_FA_SAVE u8" Export")) {
      [[maybe_unused]] auto _ = exportImage(data, 0);
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
        data.encode({image, static_cast<size_t>(width * height * 4)});
        stbi_image_free(image);
        tex.invalidateTextureCaches();
        delegate.commit("Import Image");
        stbi_image_free(image);
      }
    }
    // ImGui::EndMenu();
    // }
    ImGui::EndMenuBar();
  }

  if (delegate.mAffected.size() > 1) {
    ImGui::Text(
        (const char*)
            ICON_FA_EXCLAMATION_TRIANGLE u8" Image Properties do not support "
                                         u8"multi-selection currently.");
  }

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

void ImageActionsInstaller() {
  kpi::ActionMenuManager::get().addMenu(std::make_unique<ImageActions>());
}

} // namespace libcube::UI
