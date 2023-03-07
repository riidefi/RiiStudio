#include "G3dSrtView.hpp"

namespace riistudio::g3d {

bool Checkbox(const char* name, bool init) {
  ImGui::Checkbox(name, &init);
  return init;
}

librii::g3d::SrtFlags DrawSrtFlags(const librii::g3d::SrtFlags& flags) {
  auto result = flags;

  result.Unknown = Checkbox("Unknown"_j, flags.Unknown);
  result.Unknown = Checkbox("Identity Sclaing"_j, flags.Unknown);
  result.Unknown = Checkbox("Identity Rotation"_j, flags.Unknown);
  result.Unknown = Checkbox("Identity Translation"_j, flags.Unknown);
  result.Unknown = Checkbox("Isotropic Sclaing"_j, flags.Unknown);

  return result;
}

librii::g3d::SrtAnimationArchive
DrawSrtOptions(const librii::g3d::SrtAnimationArchive& init) {
  auto result = init;

  int frame_duration = result.frameDuration;
  ImGui::InputInt("Frame duration"_j, &frame_duration);
  result.frameDuration = frame_duration;

  int xform_model = result.xformModel;
  ImGui::Combo("Transform model"_j, &xform_model,
               "Maya\0"
               "XSI\0"
               "Max\0"_j);
  result.xformModel = xform_model;

  result.wrapMode = imcxx::Combo("Temporal wrap mode"_j, result.wrapMode,
                                 "Clamp\0"
                                 "Repeat\0"_j);

  return result;
}

void drawProperty(kpi::PropertyDelegate<SRT0>& dl, G3dSrtOptionsSurface) {
  auto& srt = dl.getActive();

  // TODO: Can be expensive to copy
  auto edited = DrawSrtOptions(srt);

  KPI_PROPERTY_EX(dl, frameDuration, edited.frameDuration);
  KPI_PROPERTY_EX(dl, xformModel, edited.xformModel);
  KPI_PROPERTY_EX(dl, wrapMode, edited.wrapMode);

  std::vector<uint8_t> is_open(
      srt.materials.size()); // Since vector<bool> isn't aliasable by bool*
  std::fill(is_open.begin(), is_open.end(), true);
  static_assert(sizeof(*&is_open[0]) == sizeof(bool));
  if (ImGui::BeginTabBar("Animations")) {
    for (size_t i = 0; i < srt.materials.size(); ++i) {
      auto& anim = srt.materials[i];
      char buf[64];
      snprintf(buf, sizeof(buf), "Animation %u => targets Material %s",
               static_cast<u32>(i), anim.name.c_str());
      if (ImGui::BeginTabItem(buf, reinterpret_cast<bool*>(&is_open[i]))) {
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
  for (size_t i = 0; i < srt.materials.size(); ++i) {
    if (!is_open[i]) {
      srt.materials.erase(srt.materials.begin() + i);
      dl.commit("Deleted SRT animation");
      break;
    }
  }
}

} // namespace riistudio::g3d
