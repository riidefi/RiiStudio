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

uint32_t fnv1a_32_hash(std::string_view text) {
  uint32_t hash = 0x811c9dc5;
  uint32_t prime = 0x1000193;

  for (int i = 0; i < text.length(); i++) {
    hash = hash ^ text[i];
    hash = hash * prime;
  }

  return hash;
}

void ShowSrtAnim(librii::g3d::SrtAnimationArchive& anim) {
  int maxTrackSize = 0;
  for (auto& targetedMtx : anim.matrices) {
    for (auto& track : {targetedMtx.matrix.scaleX, targetedMtx.matrix.scaleY,
                        targetedMtx.matrix.rot, targetedMtx.matrix.transX,
                        targetedMtx.matrix.transY}) {
      if (track.size() > maxTrackSize) {
        maxTrackSize = track.size();
      }
    }
  }

  if (ImGui::BeginTable("SrtAnim Table", maxTrackSize + 3,
                        ImGuiTableFlags_Borders)) {
    ImGui::TableSetupColumn("Material");
    ImGui::TableSetupColumn("Target");
    for (int i = 0; i < maxTrackSize; ++i) {
      ImGui::TableSetupColumn(("Column " + std::to_string(i)).c_str());
    }
    ImGui::TableHeadersRow();
    for (auto& targetedMtx : anim.matrices) {
      int targetMtxId = &targetedMtx - anim.matrices.data();
      auto matName = targetedMtx.target.materialName;
      u32 matNameHash = fnv1a_32_hash(matName);
      u32 matColor = (matNameHash << 8) | 0xff;

      for (auto* track :
           {&targetedMtx.matrix.scaleX, &targetedMtx.matrix.scaleY,
            &targetedMtx.matrix.rot, &targetedMtx.matrix.transX,
            &targetedMtx.matrix.transY}) {
        int track_idx = track - &targetedMtx.matrix.scaleX;
        auto tgtId = static_cast<librii::g3d::SRT0Matrix::TargetId>(track_idx);
        for (int j = 0; j < 5; ++j) {
          ImGui::TableNextRow();
          if (j == 0) {
            ImGui::TableSetColumnIndex(0);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            ImGui::Text("%s", targetedMtx.target.materialName.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            ImGui::Text("%s%d: %s",
                        targetedMtx.target.indirect ? "IndMtx" : "TexMtx",
                        targetedMtx.target.matrixIndex,
                        // string_view created from cstring, so .data() is
                        // null-terminated
                        magic_enum::enum_name(tgtId).data());
          }
          for (int i = 0; i < track->size(); ++i) {
            auto& keyframe = (*track)[i];
            ImGui::TableSetColumnIndex(i + 2);
            auto kfStringId = std::format("{}{}{}", targetMtxId, track_idx, i);

            if (j == 0) {
              ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
              ImGui::Text("Keyframe %d", i);
              ImGui::SameLine();
              auto title = std::format(
                  "{}##{}", (const char*)ICON_FA_TIMES_CIRCLE, kfStringId);
              ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
              if (ImGui::Button(title.c_str())) {
                track->erase(track->begin() + i);
              }
            } else if (j == 1) {
              ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
              ImGui::InputFloat(("Frame##" + kfStringId).c_str(),
                                &keyframe.frame);
            } else if (j == 2) {
              ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
              ImGui::InputFloat(("Value##" + kfStringId).c_str(),
                                &keyframe.value);
            } else if (j == 3) {
              ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
              ImGui::InputFloat(("Tangent##" + kfStringId).c_str(),
                                &keyframe.tangent);
            }
          }
          if (j == 0) {
            ImGui::TableSetColumnIndex(track->size() + 2);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            auto title = std::format("{}##{}{}", "+", track_idx, targetMtxId);
            auto avail = ImVec2{ImGui::GetContentRegionAvail().x, 0.0f};
            if (ImGui::Button(title.c_str(), avail)) {
              // Define a new keyframe with default values
              librii::g3d::SRT0KeyFrame newKeyframe;
              newKeyframe.frame = 0.0f;
              newKeyframe.value = 0.0f; // TODO: Grab last if exist
              newKeyframe.tangent = 0.0f;

              track->push_back(newKeyframe);
            }
          }
        }
      }
    }

    ImGui::EndTable();
  }
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

  ShowSrtAnim(result);

  return result;
}

void drawProperty(kpi::PropertyDelegate<SRT0>& dl, G3dSrtOptionsSurface) {
  SRT0& srt = dl.getActive();

  // TODO: Can be expensive to copy
  auto edited = DrawSrtOptions(srt);

  KPI_PROPERTY_EX(dl, frameDuration, edited.frameDuration);
  KPI_PROPERTY_EX(dl, xformModel, edited.xformModel);
  KPI_PROPERTY_EX(dl, wrapMode, edited.wrapMode);

  if (edited != srt) {
    static_cast<librii::g3d::SrtAnim&>(srt) = edited;
    dl.commit("DrawSrtOptions edit");
  }

#if 0
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
#endif
}

} // namespace riistudio::g3d
