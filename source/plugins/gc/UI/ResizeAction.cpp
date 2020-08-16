#include "ResizeAction.hpp"
#include <core/util/gui.hpp>
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

  ImGui::Combo("Algorithm", &resizealgo, "Ultimate\0Lanczos\0");

  if (ImGui::Button((const char*)ICON_FA_CHECK u8" Resize")) {
    printf("Do the resizing..\n");

    const auto oldWidth = data.getWidth();
    const auto oldHeight = data.getHeight();

    data.setWidth(resize[0].value);
    data.setHeight(resize[1].value);
    data.resizeData();

    libcube::image_platform::transform(
        data.getData(), resize[0].value, resize[1].value,
        static_cast<libcube::gx::TextureFormat>(data.getTextureFormat()),
        std::nullopt, data.getData(), oldWidth, oldHeight,
        data.getMipmapCount(),
        static_cast<libcube::image_platform::ResizingAlgorithm>(resizealgo));
    if (changed != nullptr)
      *changed = true;

    return false;
  }
  ImGui::SameLine();

  if (ImGui::Button((const char*)ICON_FA_CROSS u8" Cancel")) {
    return false;
  }
  return true;
}

bool ReformatAction::reformat_draw(Texture& data, bool* changed) {
  if (reformatOpt == -1)
    reformatOpt = data.getTextureFormat();

  ImGui::InputInt("Format", &reformatOpt);
  if (ImGui::Button((const char*)ICON_FA_CHECK u8" Okay")) {
    const auto oldFormat = data.getTextureFormat();
    data.setTextureFormat(reformatOpt);
    data.resizeData();

    libcube::image_platform::transform(
        data.getData(), data.getWidth(), data.getHeight(),
        static_cast<libcube::gx::TextureFormat>(oldFormat),
        static_cast<libcube::gx::TextureFormat>(reformatOpt), data.getData(),
        data.getWidth(), data.getHeight(), data.getMipmapCount() - 1);

    if (changed != nullptr)
      *changed = true;
    return false;
  }
  ImGui::SameLine();

  if (ImGui::Button((const char*)ICON_FA_CROSS u8" Cancel")) {
    return false;
  }

  return true;
}

} // namespace libcube::UI
