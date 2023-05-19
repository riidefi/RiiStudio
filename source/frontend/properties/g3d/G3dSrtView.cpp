#include "G3dSrtView.hpp"
#include <rsl/Defer.hpp>

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

// In number of keyframes
static int getMaxTrackSize(const librii::g3d::SrtAnimationArchive& anim) {
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
  return maxTrackSize;
}

void ShowSrtAnim(librii::g3d::SrtAnimationArchive& anim, Filter& visFilter,
                 const riistudio::g3d::Model& mdl) {
  std::unordered_set<std::string> animatedMaterials;
  for (auto& mtx : anim.matrices) {
    animatedMaterials.emplace(mtx.target.materialName);
  }
  // Visibility selector for materials
  //
  // *--------------*------------*
  // | Material     | Animated?  |
  // *--------------|------------*
  // | boost        |    [X]     |
  // *--------------|------------*
  // | ef_dushboard |    [X]     |
  // *--------------|------------*
  // |     ...      |    ...     |
  // *--------------|------------*
  // | ef_sea       |    [X]     |
  // *--------------*------------*
  //
  // Coloring:
  // - Foreground: Animated materials BLACK, others WHITE
  // - Background: By material name hash. Matches keyframe table cells.
  //
  ImGui::BeginChild("ChildL", ImVec2(150, 0.0f));
  auto flags = ImGuiTableFlags_Borders;
  if (ImGui::BeginTable("Materials", 2, flags)) {
    ImGui::TableSetupColumn("Material", ImGuiTableColumnFlags_WidthStretch, 5);
    ImGui::TableSetupColumn("A?", ImGuiTableColumnFlags_WidthStretch, 2);
    ImGui::TableHeadersRow();
    for (auto& [matName, visible] : visFilter.materials()) {
      bool animated = animatedMaterials.contains(matName);
      u32 matNameHash = fnv1a_32_hash(matName);
      // ABGR encoding for some reason
      u32 matColor = matNameHash | 0xff00'0000;
      u32 textFg = animated ? (0xFF00'0000) : 0xFFFF'FFFF;
      ImGui::PushStyleColor(ImGuiCol_Text, textFg);
      RSL_DEFER(ImGui::PopStyleColor());
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      ImGui::Text("%s", matName.c_str());
      ImGui::TableSetColumnIndex(1);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      char buf[32]{};
      std::format_to_n(buf, sizeof(buf) - 1, "##enabled{}", matName);
      ImGui::Checkbox(buf, &visible);
    }
    ImGui::EndTable();
  }
  ImGui::EndChild();
  ImGui::SameLine();
  //
  // Our table editor is given by the following matrix:
  //
  // *----------*---------*------------*-----*------------*---------*
  // | Material | Target  | Keyframe 1 | ... | Keyframe N | +Button |
  // *----------|---------|------------|-----|------------|---------*
  // | MName    | Mtx0ROT | Frame      | ... | Frame      |    +    |
  // *----------|---------*------------|-----|------------*---------*
  // .          .         | Value      | ... | Value      |         .
  // .....................*------------|-----|------------*..........
  // .          .         | Tangent    | ... | Tangent    |         .
  // .....................*------------*-----*------------*..........
  //                                                                 [N+3 x
  //                                                                 3T+1]
  // Note its width is N+3, where N is the maximum track size.
  // Note its height is 3T+1 where T is the number of tracks.
  //
  // ImGui is Row-Dominant, so we traverse in this order:
  //   AnimMtx -> FVT -> KeyFrame
  // When using filter, it becomes
  //   (Material -> Tgt) -> FVT -> KeyFrame
  //
  struct FVT {
    std::string matName;
    u32 color = 0xFF;
    size_t mtxId = 0;
    u32 subtrack = 0;
  };
  std::vector<FVT> fvtsToDraw;
  for (auto& [matName, visible] : visFilter.materials()) {
    if (!visible) {
      continue;
    }
    u32 matNameHash = fnv1a_32_hash(matName);
    // ABGR encoding for some reason
    u32 matColor = matNameHash | 0xff00'0000;
    for (size_t mtxId = 0; mtxId < 11; ++mtxId) {
      bool mtxVisible = *visFilter.attr(mtxId);
      if (!mtxVisible) {
        continue;
      }
      // sx, sy, r, tx, ty
      for (u32 subtrack = 0; subtrack < 5; ++subtrack) {
        fvtsToDraw.push_back(FVT{
            .matName = matName,
            .color = matColor,
            .mtxId = mtxId,
            .subtrack = subtrack,
        });
      }
    }
  }
  int maxTrackSize = getMaxTrackSize(anim);

  ImGui::BeginChild("ChildR");
  if (ImGui::BeginTable("SrtAnim Table", maxTrackSize + 3,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX)) {
    ImGui::TableSetupColumn("Material");
    ImGui::TableSetupColumn("Target");
    for (int i = 0; i < maxTrackSize; ++i) {
      ImGui::TableSetupColumn(("KeyFrame " + std::to_string(i)).c_str());
    }
    ImGui::TableHeadersRow();

    for (auto& fvt : fvtsToDraw) {
      int targetMtxId = fvt.mtxId;
      auto matName = fvt.matName;
      u32 matColor = fvt.color;

      librii::g3d::SrtAnim::Target target{
          .materialName = matName,
          .indirect = (targetMtxId >= 8),
          .matrixIndex = (targetMtxId % 8),
      };
      ptrdiff_t it = std::distance(
          anim.matrices.begin(),
          std::find_if(anim.matrices.begin(), anim.matrices.end(),
                       [&](auto& x) { return x.target == target; }));

      u32 subtrack = fvt.subtrack;
      librii::g3d::SrtAnim::Track* track = nullptr;
      if (it != anim.matrices.size()) {
        track = &(&anim.matrices[it].matrix.scaleX)[subtrack];
      }
      // 0: INFO
      // 1: Frame
      // 2: Value
      // 3: Tangent
      for (int row_index = 0; row_index < 4; ++row_index) {
        ImGui::TableNextRow();
        if (row_index == 0) {
          // Row 0: Frame + INFO
          ImGui::TableSetColumnIndex(0);
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
          ImGui::Text("%s", matName.c_str());
          ImGui::TableSetColumnIndex(1);
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
          // string_view created from cstring, so
          // .data() is null-terminated
          auto tgtId = static_cast<librii::g3d::SRT0Matrix::TargetId>(subtrack);
          const char* subtrackName = magic_enum::enum_name(tgtId).data();
          ImGui::Text("%s%d: %s", targetMtxId >= 8 ? "IndMtx" : "TexMtx",
                      (targetMtxId % 8), subtrackName);
        }
        for (size_t i = 0; i < (track ? track->size() : 0); ++i) {
          auto& keyframe = track->at(i);
          auto kfStringId =
              std::format("{}{}{}{}", matName, targetMtxId, subtrack, i);
          ImGui::PushID(kfStringId.c_str());
          RSL_DEFER(ImGui::PopID());

          ImGui::TableSetColumnIndex(i + 2);
          if (row_index == 0) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            ImGui::Text("Keyframe %d", static_cast<int>(i));
            ImGui::SameLine();
            auto title = std::format(
                "{}##{}", (const char*)ICON_FA_TIMES_CIRCLE, kfStringId);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            if (ImGui::Button(title.c_str())) {
              track->erase(track->begin() + i);
            }
          } else if (row_index == 1) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            ImGui::InputFloat("Frame", &keyframe.frame);
          } else if (row_index == 2) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            ImGui::InputFloat("Value", &keyframe.value);
          } else if (row_index == 3) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
            ImGui::InputFloat("Tangent", &keyframe.tangent);
          }
        }
        if (row_index == 0) {
          ImGui::TableSetColumnIndex((track ? track->size() : 0) + 2);
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
          auto title = std::format("{}##{}{}{}", "    +    ", matName, subtrack,
                                   targetMtxId);
          auto avail = ImVec2{ImGui::GetContentRegionAvail().x, 0.0f};
          if (ImGui::Button(title.c_str(), avail)) {
            // Define a new keyframe with default values
            librii::g3d::SRT0KeyFrame newKeyframe;
            newKeyframe.frame = 0.0f;
            newKeyframe.value = 0.0f; // TODO: Grab last if exist
            newKeyframe.tangent = 0.0f;

            if (!track) {
              auto& mtx = anim.matrices.emplace_back();
              mtx.target = target;
              track = &(&mtx.matrix.scaleX)[subtrack];
            }
            track->push_back(newKeyframe);
          }
        }
      }
    }

    ImGui::EndTable();
  }
  ImGui::EndChild();
}
librii::g3d::SrtAnimationArchive
DrawSrtOptions(const librii::g3d::SrtAnimationArchive& init, Filter& visFilter,
               const riistudio::g3d::Model& mdl) {
  auto result = init;

  char buf[256]{};
  snprintf(buf, sizeof(buf), "%s", init.name.c_str());
  ImGui::InputText("Name", buf, sizeof(buf) - 1);
  result.name = buf;

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

  ShowSrtAnim(result, visFilter, mdl);

  return result;
}

void drawProperty(kpi::PropertyDelegate<SRT0>& dl,
                  G3dSrtOptionsSurface& surface) {
  SRT0& srt = dl.getActive();

  auto& mdl = *dynamic_cast<const riistudio::g3d::Model*>(
      srt.childOf->folderAt(0)->atObject(0));
  if (!surface.m_filterReady) {
    surface.m_filter.initFromScene(mdl, srt);
    surface.m_filterReady = true;
  }

  // TODO: Can be expensive to copy
  auto edited = DrawSrtOptions(srt, surface.m_filter, mdl);

  KPI_PROPERTY_EX(dl, frameDuration, edited.frameDuration);
  KPI_PROPERTY_EX(dl, xformModel, edited.xformModel);
  KPI_PROPERTY_EX(dl, wrapMode, edited.wrapMode);

  if (edited != srt) {
    static_cast<librii::g3d::SrtAnim&>(srt) = edited;
    dl.commit("DrawSrtOptions edit");
  }
}

} // namespace riistudio::g3d
