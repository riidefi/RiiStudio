#pragma once

#include <brres/lib/brres-sys/include/brres_sys.h>
#include <core/util/timestamp.hpp>
#include <librii/image/ImagePlatform.hpp>
#include <librii/szs/SZS.hpp>
#include <rsmeshopt/include/rsmeshopt.h>

namespace riistudio::frontend {

void DrawVersionInfo() {
  ImGui::Text("%s", RII_TIME_STAMP);
  std::string gctex_ver =
      "gctex " + std::string(librii::image::gctex_version());
  ImGui::Text("%s", gctex_ver.c_str());
  std::string szs_ver = "szs " + std::string(librii::szs::szs_version());
  ImGui::Text("%s", szs_ver.c_str());

  std::string brres_sys_ver = brres::get_version();
  ImGui::Text("brres %s", brres_sys_ver.c_str());

  std::string rsmeshopt_ver = rsmeshopt::get_version();
  ImGui::Text("rsmeshopt %s", rsmeshopt_ver.c_str());
}

} // namespace riistudio::frontend
