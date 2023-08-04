#pragma once

#include <librii/image/ImagePlatform.hpp>

namespace riistudio::frontend {

void DrawVersionInfo() {
  std::string gctex_ver =
      "gctex " + std::string(librii::image::gctex_version());
  ImGui::Text("%s", gctex_ver.c_str());
}

} // namespace riistudio::frontend
