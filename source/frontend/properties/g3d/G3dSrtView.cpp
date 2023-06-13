#include "G3dSrtView.hpp"
#include <rsl/Defer.hpp>

#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

static uint32_t fnv1a_32_hash(std::string_view text) {
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

static void
VisibilityMatrixIDSelector(Filter& visFilter,
                           std::unordered_set<std::string>& animatedMaterials,
                           std::array<bool, 8 + 3>& animatedMtxSlots) {
  ImGui::BeginChild("ChildL", ImVec2(150, 0.0f));
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
  auto flags = ImGuiTableFlags_Borders;
  ImGui::BeginChild("ChildU",
                    ImVec2(0, ImGui::GetContentRegionAvail().y - 150));
  if (ImGui::BeginTable("Materials", 2, flags)) {
    ImGui::TableSetupColumn("Material", ImGuiTableColumnFlags_WidthStretch, 5);
    ImGui::TableSetupColumn("A?", ImGuiTableColumnFlags_WidthStretch, 2);
    ImGui::TableHeadersRow();
    int i = 0;
    for (auto& [matName, visible] : visFilter.materials()) {
      ImGui::PushID(i++);
      RSL_DEFER(ImGui::PopID());
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
      ImGui::Checkbox("##enabled", &visible);
    }
    ImGui::EndTable();
  }
  ImGui::EndChild();

  // MatrixID selector

  ImGui::BeginChild("ChildD");
  if (ImGui::BeginTable("Materials", 2, flags)) {
    ImGui::TableSetupColumn("Matrix", ImGuiTableColumnFlags_WidthStretch, 5);
    ImGui::TableSetupColumn("A?", ImGuiTableColumnFlags_WidthStretch, 2);
    ImGui::TableHeadersRow();
    for (int i = 0; i < 8 + 3; ++i) {
      ImGui::PushID(i);
      RSL_DEFER(ImGui::PopID());
      bool animated = animatedMtxSlots[i];
      u32 textFg = animated ? (0xFF00'00FF) : 0xFFFF'FFFF;
      ImGui::PushStyleColor(ImGuiCol_Text, textFg);
      RSL_DEFER(ImGui::PopStyleColor());
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s%d", (i >= 8 ? "IndMtx" : "TexMtx"), (i % 8));
      ImGui::TableSetColumnIndex(1);
      ImGui::Checkbox("##enabled", visFilter.attr(i));
    }
    ImGui::EndTable();
  }
  ImGui::EndChild();

  ImGui::EndChild();
}

struct FVT {
  std::string matName;
  u32 color = 0xFF;
  size_t mtxId = 0;
  u32 subtrack = 0;
};
static std::vector<FVT> GatherFVTs(Filter& visFilter) {
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
  return fvtsToDraw;
}
static void CurveEditorWindow(librii::g3d::SrtAnimationArchive& anim,
                              Filter& visFilter, float frame_duration) {
  //
  // Our curve table editor is given by the following matrix:
  //
  // *----------*---------*------------*--------------*
  // | Material | Target  | Keyframe X | Curve editor |
  // *----------|---------|------------|--------------|
  // | MName    | Mtx0ROT | Frame      |     ...      |
  // *----------|---------*------------|--------------|
  // .          .         | Value      |     ...      |
  // .....................*------------|--------------|
  // .          .         | Tangent    |     ...      |
  // .....................*------------*--------------*
  //                                                  [4 x 3T+1]
  // Note its width is 4.
  // Note its height is 3T+1 where T is the number of tracks.
  //
  // ImGui is Row-Dominant, so we traverse in this order:
  //   AnimMtx -> FVT -> KeyFrame
  // When using filter, it becomes
  //   (Material -> Tgt) -> FVT -> KeyFrame
  //
  auto fvtsToDraw = GatherFVTs(visFilter);

  ImGui::BeginChild("ChildR");
  if (ImGui::BeginTable("SrtAnim Table", 4,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX)) {
    ImGui::TableSetupColumn("Material");
    ImGui::TableSetupColumn("Target");
    ImGui::TableSetupColumn("Selected KeyFrame");
    ImGui::TableSetupColumn("Curve", ImGuiTableColumnFlags_WidthStretch);
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
      ImGui::TableNextRow();
      {
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

      auto kfStringId = std::format("{}{}{}", matName, targetMtxId, subtrack);

      float top_of_row = ImGui::GetCursorPosY();

      static auto editor_selections{
          std::unordered_map<std::string, KeyframeIndexSelection>()};

      KeyframeIndexSelection selection;

      auto found = editor_selections.find(kfStringId);
      if (found == editor_selections.end()) {
        selection = KeyframeIndexSelection();
      } else {
        selection = editor_selections.at(found->first);
      }

      if (track == nullptr) {
        selection.clear();
      }

      int active_idx = -1;
      auto active = selection.get_active();
      if (active) {
        active_idx = *active;
      }

      ImGui::PushID(kfStringId.c_str());
      RSL_DEFER(ImGui::PopID());

      ImGui::TableSetColumnIndex(2);

      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);

      if (active_idx > -1) {
        ImGui::Text("Keyframe %d", static_cast<int>(active_idx));
        ImGui::SameLine();
        auto title = std::format("{}##{}", (const char*)ICON_FA_TIMES_CIRCLE,
                                 kfStringId);

        auto& keyframe = track->at(active_idx);

        bool delete_keyframe = false;

        if (ImGui::Button(title.c_str())) {
          track->erase(track->begin() + active_idx);
          delete_keyframe = true;
        }
        ImGui::InputFloat("Frame", &keyframe.frame);
        ImGui::InputFloat("Value", &keyframe.value);
        ImGui::InputFloat("Tangent", &keyframe.tangent);

        // adjust the selected keyframes if necessary
        if (delete_keyframe)
          selection.deselect(active_idx);

      } else {
        ImGui::Dummy(ImVec2(0, 100));
      }

      ImGui::NewLine();

      float row_height = ImGui::GetCursorPosY() - top_of_row;
      ImGui::TableSetColumnIndex(3);
      auto size = ImGui::GetContentRegionAvail();
      size.y = row_height;

      if (track) {
        srt_curve_editor(kfStringId, size, track, frame_duration, &selection);
        editor_selections.insert_or_assign(kfStringId, selection);
      } else {
        ImGui::PushID(kfStringId.c_str());

        if (ImGui::Button("Add animation", size)) {
          auto& mtx = anim.matrices.emplace_back();
          mtx.target = target;
          track = &(&mtx.matrix.scaleX)[subtrack];
        }

        ImGui::PopID();
      }
    }

    ImGui::EndTable();
  }
  ImGui::EndChild();
}
static void TableEditorWindow(librii::g3d::SrtAnimationArchive& anim,
                              Filter& visFilter, float frame_duration) {
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
  auto fvtsToDraw = GatherFVTs(visFilter);
  int maxTrackSize = getMaxTrackSize(anim);

  ImGui::BeginChild("ChildR");
  if (ImGui::BeginTable("SrtAnim Table", maxTrackSize + 3,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX)) {
    ImGui::TableSetupColumn("Material");
    ImGui::TableSetupColumn("Target");
    for (int i = 0; i < maxTrackSize + 1; ++i) {
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
void ShowSrtAnim(librii::g3d::SrtAnimationArchive& anim, Filter& visFilter,
                 const riistudio::g3d::Model& mdl, float frame_duration) {
  std::unordered_set<std::string> animatedMaterials;
  std::array<bool, 8 + 3> animatedMtxSlots{};
  for (auto& mtx : anim.matrices) {
    auto& target = mtx.target;
    animatedMaterials.emplace(target.materialName);
    animatedMtxSlots[target.matrixIndex + (target.indirect ? 8 : 0)] = true;
  }

  if (ImGui::BeginTabBar("Views")) {
    if (ImGui::BeginTabItem((const char*)ICON_FA_BEZIER_CURVE "CURVE EDITOR")) {
      VisibilityMatrixIDSelector(visFilter, animatedMaterials,
                                 animatedMtxSlots);
      ImGui::SameLine();
      CurveEditorWindow(anim, visFilter, frame_duration);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((const char*)ICON_FA_TABLE "TABLE EDITOR")) {
      VisibilityMatrixIDSelector(visFilter, animatedMaterials,
                                 animatedMtxSlots);
      ImGui::SameLine();
      TableEditorWindow(anim, visFilter, frame_duration);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
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

  ShowSrtAnim(result, visFilter, mdl, frame_duration);

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
