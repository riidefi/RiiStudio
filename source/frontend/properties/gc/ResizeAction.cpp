#include "ResizeAction.hpp"
#include <core/util/gui.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/Texture.hpp>

namespace libcube::UI {

void ResizeAction::resize_reset() {
  resize[0].value = -1;
  resize[1].value = -1;
  resize[0].before = -1;
  resize[1].before = -1;
}

bool ResizeAction::resize_draw(Texture& data, bool* changed) {
  if (resize[0].before <= 0)
    resize[0].before = data.getWidth();
  if (resize[1].before <= 0)
    resize[1].before = data.getHeight();

  int dX = 0;
  for (auto& it : resize) {
    auto& other = dX++ ? resize[0] : resize[1];

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

  if (resize[0].value <= 0) {
    resize[0].value = data.getWidth();
  } else if (resize[0].value > 1024) {
    resize[0].value = 1024;
  }
  if (resize[1].value <= 0) {
    resize[1].value = data.getHeight();
  } else if (resize[1].value > 1024) {
    resize[1].value = 1024;
  }

  resizealgo = imcxx::Combo("Algorithm", resizealgo,
                            "Ultimate\0"
                            "Lanczos\0");

  if (ImGui::Button((const char*)ICON_FA_CHECK u8" Resize")) {
    printf("Do the resizing..\n");

    const auto oldWidth = data.getWidth();
    const auto oldHeight = data.getHeight();

    data.setWidth(resize[0].value);
    data.setHeight(resize[1].value);
    data.resizeData();

    librii::image::transform(data.getData(), resize[0].value, resize[1].value,
                             data.getTextureFormat(), std::nullopt,
                             data.getData(), oldWidth, oldHeight,
                             data.getMipmapCount(), resizealgo);
    if (changed != nullptr)
      *changed = true;

    return false;
  }
  ImGui::SameLine();

  if (ImGui::Button((const char*)ICON_FA_TIMES u8" Cancel")) {
    return false;
  }
  return true;
}

librii::gx::TextureFormat TexFormatCombo(librii::gx::TextureFormat format) {
  int format_int = static_cast<int>(format);

  if (format_int == 14) // Temporarily set CMPR to 7 for UI convenience.
    format_int = 7;

  ImGui::Combo("Texture Format", &format_int,
               "I4\0"
               "I8\0"
               "IA4\0"
               "IA8\0"
               "RGB565\0"
               "RGB5A3\0"
               "RGBA8\0"
               "CMPR\0");

  if (format_int == 7)
    format_int = 14; // Set CMPR back to its respective ID.

  return static_cast<librii::gx::TextureFormat>(format_int);
}

bool ReformatAction::reformat_draw(Texture& data, bool* changed) {
  if (reformatOpt == -1)
    reformatOpt = static_cast<int>(data.getTextureFormat());

  reformatOpt = static_cast<int>(
      TexFormatCombo(static_cast<librii::gx::TextureFormat>(reformatOpt)));

  if (ImGui::Button((const char*)ICON_FA_CHECK u8" Okay")) {
    const auto oldFormat = data.getTextureFormat();
    data.setTextureFormat(static_cast<librii::gx::TextureFormat>(reformatOpt));
    data.resizeData();

    librii::image::transform(
        data.getData(), data.getWidth(), data.getHeight(), oldFormat,
        static_cast<librii::gx::TextureFormat>(reformatOpt), data.getData(),
        data.getWidth(), data.getHeight(), data.getMipmapCount());

    if (changed != nullptr)
      *changed = true;
    return false;
  }
  ImGui::SameLine();

  if (ImGui::Button((const char*)ICON_FA_TIMES u8" Cancel")) {
    return false;
  }

  return true;
}

} // namespace libcube::UI
