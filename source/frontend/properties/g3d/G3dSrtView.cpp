#include "G3dSrtView.hpp"
#include <rsl/Defer.hpp>

#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {
enum SubTrack {
  SCALE_X,
  SCALE_Y,
  ROT,
  TRANS_X,
  TRANS_Y,
};

struct TrackID {
  std::string matName;
  size_t mtxId = 0;
  SubTrack subtrack = SCALE_X;

  bool operator==(const TrackID& other) const = default;
};

struct TrackUIInfo {
  std::string matName;
  u32 color = 0xFF;
  size_t mtxId = 0;
  SubTrack subtrack = SCALE_X;
};
} // namespace riistudio::g3d

namespace std {
template <> struct hash<riistudio::g3d::TrackID> {
  size_t operator()(const riistudio::g3d::TrackID& k) const {
    size_t h1 = std::hash<std::string>{}(k.matName);
    size_t h2 = std::hash<size_t>{}(k.mtxId);
    size_t h3 = std::hash<int>{}(k.subtrack);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};
} // namespace std

namespace riistudio::g3d {

#define rgb(r, g, b) (u32)(0x##r | 0x##g << 8 | 0x##b << 16 | 0xFF00'0000)

static const u32 subtrackColors[5]{
    // Scale
    rgb(FF, 55, 00),
    rgb(00, FF, FF),
    // Rotate
    rgb(00, FF, 00),
    // Translate
    rgb(FF, 00, 00),
    rgb(00, 88, FF),
};

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

static librii::g3d::SrtAnim::Track&
AddNewButton_FromTrackUIInfo(librii::g3d::SrtAnimationArchive& anim,
                             const TrackUIInfo& info) {
  librii::g3d::SrtAnim::Target target{
      .materialName = info.matName,
      .indirect = (info.mtxId >= 8),
      .matrixIndex = static_cast<int>(info.mtxId % 8),
  };
  ptrdiff_t it =
      std::distance(anim.matrices.begin(),
                    std::find_if(anim.matrices.begin(), anim.matrices.end(),
                                 [&](auto& x) { return x.target == target; }));
  assert(it == anim.matrices.size() && "Animation track already exists!");
  auto& mtx = anim.matrices.emplace_back();
  mtx.target = target;
  return mtx.matrix.subtrack(info.subtrack);
}

static void
AnimationTrackTreeview(librii::g3d::SrtAnimationArchive& anim,
                       Filter& visFilter,
                       std::unordered_set<std::string>& animatedMaterials,
                       std::array<bool, 8 + 3>& animatedMtxSlots,
                       std::optional<TrackID>& active_track) {
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

  ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x33FF'FFFF);
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0x55FF'FFFF);
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0x99FF'FFFF);
  ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFF'FFFF);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);

  auto flags = ImGuiTableFlags_Borders;
  if (ImGui::BeginTable("Materials", 2, flags)) {
    ImGui::TableSetupColumn("Curve", ImGuiTableColumnFlags_WidthStretch, 5);
    ImGui::TableSetupColumn("Visible", ImGuiTableColumnFlags_WidthStretch, 2);
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
      bool _visible = visible;
      auto flags = (_visible ? 0 : ImGuiTreeNodeFlags_Leaf) |
                   ImGuiTreeNodeFlags_FramePadding |
                   ImGuiTreeNodeFlags_SpanFullWidth;
      bool open = ImGui::TreeNodeEx(matName.c_str(), flags);

      ImGui::TableSetColumnIndex(1);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      ImGui::Checkbox("##enabled", &visible);
      if (!open)
        continue;
      RSL_DEFER(ImGui::TreePop());

      if (!_visible)
        continue;

      for (int j = 0; j < 8 + 3; ++j) {
        ImGui::PushID(j);
        RSL_DEFER(ImGui::PopID());

        bool animated = animatedMtxSlots[j];
        u32 textFg = animated ? (0xFF00'00FF) : 0xFFFF'FFFF;
        ImGui::PushStyleColor(ImGuiCol_Text, textFg);
        RSL_DEFER(ImGui::PopStyleColor());
        ImGui::TableNextRow();

        if (!animated) {
          ImGui::TableSetColumnIndex(0);
          if (ImGui::Button(std::format("Add {}{}",
                                        (j >= 8 ? "IndMtx" : "TexMtx"), (j % 8))
                                .c_str())) {

            auto info = TrackUIInfo{
                .matName = matName, .color = matColor, .mtxId = (size_t)j};
            AddNewButton_FromTrackUIInfo(anim, info);
          }

          continue;
        }

        ImGui::TableSetColumnIndex(0);
        bool* mtxVisible = visFilter.attr(j);
        bool _visible = *mtxVisible;
        auto flags = (_visible ? 0 : ImGuiTreeNodeFlags_Leaf) |
                     ImGuiTreeNodeFlags_FramePadding |
                     ImGuiTreeNodeFlags_SpanFullWidth;
        bool open = ImGui::TreeNodeEx("%s%d", flags,
                                      (j >= 8 ? "IndMtx" : "TexMtx"), (j % 8));
        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("##enabled", mtxVisible);
        if (!open)
          continue;
        RSL_DEFER(ImGui::TreePop());

        if (!_visible)
          continue;

        for (int k = 0; k < 5; ++k) {
          ImGui::PushID(k);
          RSL_DEFER(ImGui::PopID());

          ImGui::TableNextRow();
          ImGui::PushStyleColor(ImGuiCol_Text, subtrackColors[k]);
          RSL_DEFER(ImGui::PopStyleColor());
          ImGui::TableSetColumnIndex(0);
          auto tgtId = static_cast<librii::g3d::SRT0Matrix::TargetId>(k);
          const char* subtrackName = magic_enum::enum_name(tgtId).data();

          auto track_id = TrackID{
              .matName = matName, .mtxId = (size_t)j, .subtrack = (SubTrack)k};

          bool is_active_track = active_track && (*active_track == track_id);

          if (ImGui::Selectable(subtrackName, is_active_track))
            active_track = track_id;

          ImGui::TableSetColumnIndex(1);
          bool x = true;
          ImGui::Checkbox("##enabled", &x);
        }
      }
    }
    ImGui::EndTable();
  }

  ImGui::PopStyleColor(4);
  ImGui::PopStyleVar();

  ImGui::EndChild();
}

/// Note: May return nullptr!
static librii::g3d::SrtAnim::Track*
ResolveTrackUIInfo(librii::g3d::SrtAnimationArchive& anim,
                   const TrackUIInfo& info) {
  librii::g3d::SrtAnim::Target target{
      .materialName = info.matName,
      .indirect = (info.mtxId >= 8),
      .matrixIndex = static_cast<int>(info.mtxId % 8),
  };
  ptrdiff_t it =
      std::distance(anim.matrices.begin(),
                    std::find_if(anim.matrices.begin(), anim.matrices.end(),
                                 [&](auto& x) { return x.target == target; }));

  if (it == anim.matrices.size()) {
    return nullptr;
  }

  return &anim.matrices[it].matrix.subtrack(info.subtrack);
}

static std::vector<TrackUIInfo> GatherTracks(Filter& visFilter) {
  std::vector<TrackUIInfo> fvtsToDraw;
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
        fvtsToDraw.push_back(TrackUIInfo{
            .matName = matName,
            .color = matColor,
            .mtxId = mtxId,
            .subtrack = static_cast<SubTrack>(subtrack),
        });
      }
    }
  }
  return fvtsToDraw;
}
static void CurveEditorWindow(librii::g3d::SrtAnimationArchive& anim,
                              Filter& visFilter, float frame_duration,
                              std::optional<TrackID>& active_track_id) {
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
  auto visibleTracks = GatherTracks(visFilter);

  int active_track_idx = -1;

  if (ImGui::BeginChild("ChildR")) {
    std::vector<EditableTrack> tracks;

    for (int i = 0; i < visibleTracks.size(); i++) {
      auto& fvt = visibleTracks[i];
      int targetMtxId = fvt.mtxId;
      auto matName = fvt.matName;
      u32 matColor = fvt.color;
      u32 subtrack = fvt.subtrack;

      librii::g3d::SrtAnim::Track* track = ResolveTrackUIInfo(anim, fvt);
      if (track == nullptr) {
        // TODO: Double check this logic
        continue;
      }
      // string_view created from cstring, so
      // .data() is null-terminated
      auto tgtId = static_cast<librii::g3d::SRT0Matrix::TargetId>(subtrack);
      const char* subtrackName = magic_enum::enum_name(tgtId).data();

      auto track_name =
          std::format("{}{}: {}", targetMtxId >= 8 ? "IndMtx" : "TexMtx",
                      (targetMtxId % 8), subtrackName);
      EditableTrack t;
      t.track = track;
      t.name = track_name;
      t.color = subtrackColors[fvt.subtrack];
      tracks.push_back(t);

      if (active_track_id && (fvt.matName == active_track_id->matName &&
                              fvt.mtxId == active_track_id->mtxId &&
                              fvt.subtrack == active_track_id->subtrack)) {
        active_track_idx = i;
      }
    }

    if (active_track_idx == -1)
      active_track_id = std::nullopt;

    float top_of_row = ImGui::GetCursorPosY();

    static auto editor_selections{
        std::unordered_map<TrackID, KeyframeIndexSelection>()};

    std::optional<KeyframeIndexSelection> selection = std::nullopt;
    std::optional<EditableTrack> active_track = std::nullopt;

    {
      int active_keyframe_idx = -1;

      if (active_track_id) {
        active_track = tracks[active_track_idx];
        auto found = editor_selections.find(*active_track_id);
        if (found == editor_selections.end()) {
          selection = KeyframeIndexSelection();
        } else {
          selection = editor_selections.at(found->first);
        }

        auto active = selection->get_active();
        if (active) {
          active_keyframe_idx = *active;
        }
      }

      if (active_keyframe_idx > -1 && active_track &&
          active_keyframe_idx < active_track->size()) {
        ImGui::Text("Keyframe %d", static_cast<int>(active_keyframe_idx));
        ImGui::SameLine();
        auto title = std::format("{}", (const char*)ICON_FA_TIMES_CIRCLE);

        assert(active_track);

        auto& keyframe = active_track->at(active_keyframe_idx);

        bool delete_keyframe = false;

        if (ImGui::Button(title.c_str())) {
          active_track->track->erase(active_track->track->begin() +
                                     active_keyframe_idx);
          delete_keyframe = true;
        }
        ImGui::InputFloat("Frame", &keyframe.frame);
        ImGui::InputFloat("Value", &keyframe.value);
        ImGui::InputFloat("Tangent", &keyframe.tangent);

        assert(selection);

        // adjust the selected keyframes if necessary
        if (delete_keyframe)
          selection->deselect(active_keyframe_idx);

      } else {
        active_keyframe_idx = -1;
        ImGui::Dummy(ImVec2(0, 100));
      }
    }

    ImGui::NewLine();
    auto size = ImGui::GetContentRegionAvail();

    srt_curve_editor(anim.name, size, tracks, frame_duration, active_track_idx,
                     selection ? &(*selection) : nullptr);

    if (active_track && selection)
      editor_selections.insert_or_assign(*active_track_id, *selection);

    if (active_track_idx > -1)
      active_track_id =
          TrackID{.matName = visibleTracks[active_track_idx].matName,
                  .mtxId = visibleTracks[active_track_idx].mtxId,
                  .subtrack = visibleTracks[active_track_idx].subtrack};
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
  auto visibleTracks = GatherTracks(visFilter);
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

    for (auto& fvt : visibleTracks) {
      int targetMtxId = fvt.mtxId;
      auto matName = fvt.matName;
      u32 matColor = fvt.color;

      u32 subtrack = fvt.subtrack;
      librii::g3d::SrtAnim::Track* track = ResolveTrackUIInfo(anim, fvt);
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
              track = &AddNewButton_FromTrackUIInfo(anim, fvt);
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

  static std::optional<TrackID> active_track_id{std::nullopt};

  if (ImGui::BeginTabBar("Views")) {
    if (ImGui::BeginTabItem((const char*)ICON_FA_BEZIER_CURVE "CURVE EDITOR")) {
      AnimationTrackTreeview(anim, visFilter, animatedMaterials,
                             animatedMtxSlots, active_track_id);
      ImGui::SameLine();
      CurveEditorWindow(anim, visFilter, frame_duration, active_track_id);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((const char*)ICON_FA_TABLE "TABLE EDITOR")) {
      AnimationTrackTreeview(anim, visFilter, animatedMaterials,
                             animatedMtxSlots, active_track_id);
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
                  G3dSrtOptionsSurface& surface,
                  const riistudio::g3d::Model& mdl) {
  SRT0& srt = dl.getActive();

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
