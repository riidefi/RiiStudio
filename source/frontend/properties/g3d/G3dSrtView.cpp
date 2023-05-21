#include "G3dSrtView.hpp"
#include <rsl/Defer.hpp>

namespace riistudio::g3d {

float map(float value, float minIn, float maxIn, float minOut, float maxOut) {
  return (value - minIn) * (maxOut - minOut) / (maxIn - minIn) + minOut;
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

void ShowSRTCurveEditor(std::string id, ImVec2 size,
                        librii::g3d::SrtAnim::Track* track,
                        float frame_duration) {
  auto dl = ImGui::GetWindowDrawList();

  auto top_left = ImGui::GetCursorScreenPos();
  auto bottom_right = top_left + size;

  auto rect = ImRect(top_left, bottom_right);

  auto imgui_id = ImGui::GetID(id.c_str());

  if (!ImGui::ItemAdd(rect, imgui_id))
    return;

  ImGui::ItemHoverable(rect, imgui_id);

  float min_frame = 0;
  float max_frame = frame_duration;

  float min_value = -1;
  float max_value = 1;

  float left = top_left.x;
  float top = top_left.y;
  float right = bottom_right.x;
  float bottom = bottom_right.y;

  const float sample_size = 2;

  struct CurveEditorState {
    float left_frame;
    float top_value;
    float bottom_value;
    float frame_width;
  };

  static auto editor_states{new std::map<std::string, CurveEditorState>()};

  CurveEditorState s;

  auto found = editor_states->find(id);
  if (found == editor_states->end()) {
    s.left_frame = 0;
    s.top_value = 1;
    s.bottom_value = 0;
    s.frame_width = size.x/frame_duration;
  } else {
    s = editor_states->at(found->first);
  }

  float s_right_frame = s.left_frame + size.x / s.frame_width;

  dl->PushClipRect(top_left, bottom_right, true);

  ImVec2 mouse_pos = ImGui::GetMousePos();

  float mouse_pos_value =
      map(mouse_pos.y, top, bottom, s.top_value, s.bottom_value);
  float mouse_pos_frame =
      map(mouse_pos.x, left, right, s.left_frame, s_right_frame);

  float mouse_delta_frame =
      ImGui::GetIO().MouseDelta.x * (s_right_frame - s.left_frame) / size.x;
  float mouse_delta_value =
      ImGui::GetIO().MouseDelta.y * (s.bottom_value - s.top_value) / size.y;

  bool is_hovered = ImGui::IsItemHovered();

  if (is_hovered) {
    dl->AddRect(top_left, bottom_right, 0xFFFFFFFF);

    // Zoom
    if (ImGui::GetIO().KeyCtrl) {
      float scale_factor = pow(1.05, -ImGui::GetIO().MouseWheel);

      // Horizontal
      if (ImGui::GetIO().KeyShift) {
        s.left_frame =
            mouse_pos_frame + (s.left_frame - mouse_pos_frame) * scale_factor;

        s.frame_width /= scale_factor;

        // Vertical
      } else {
        s.top_value =
            mouse_pos_value + (s.top_value - mouse_pos_value) * scale_factor;
        s.bottom_value =
            mouse_pos_value + (s.bottom_value - mouse_pos_value) * scale_factor;
      }

      // recalculate
      s_right_frame = s.left_frame + size.x / s.frame_width;
      mouse_pos_value =
          map(mouse_pos.y, top, bottom, s.top_value, s.bottom_value);

      mouse_pos_frame =
          map(mouse_pos.x, left, right, s.left_frame, s_right_frame);
    }
  }

  float mouse_x_curve_value = 0;

  if (!track->empty()) {
    int num_points = (int)(size.x / sample_size) + 1;

    ImVec2 points[num_points];

    auto first = track->front();
    auto last = track->back();

    auto previous = first;
    auto next = first;

    if (first.frame > s.left_frame) {
      previous.frame = s.left_frame;
      previous.tangent = first.tangent;
      previous.value =
          first.value + first.tangent * (s.left_frame - first.frame);
    }

    int key_frame_idx = 0;

    float previous_sample = 0;

    for (int i = 0; i < num_points; i++) {
      float frame = (i * sample_size) / s.frame_width + s.left_frame;
      float x = left + i * sample_size;

      // advance keyframe when required
      if (frame >= next.frame) {
        previous = next;

        if (++key_frame_idx >= track->size()) {
          next.frame = s_right_frame;
          next.tangent = last.tangent;
          next.value = last.value + last.tangent * (s_right_frame - last.frame);
        } else {
          next = track->at(key_frame_idx);
        }
      }

      // hermite interpolation
      float t = map(frame, previous.frame, next.frame, 0, 1);
      float inv_t = t - 1.0f; //-1 to 0

      float val = previous.value +
                  (frame - previous.frame) * inv_t *
                      (inv_t * previous.tangent + t * next.tangent) +
                  t * t * (3.0f - 2.0f * t) * (next.value - previous.value);

      points[i] = ImVec2(x, map(val, s.bottom_value, s.top_value, bottom, top));

      if (x - sample_size <= mouse_pos.x && x >= mouse_pos.x)
        mouse_x_curve_value =
            map(mouse_pos.x, x - sample_size, x, previous_sample, val);

      previous_sample = val;
    }

    dl->AddPolyline(points, num_points, 0xFFFFFFFF, ImDrawFlags_None, 1.5);
  }

  int hovered_keyframe = -1;

  for (int i = 0; i < track->size(); i++) {
    auto key_frame = track->at(i);
    ImVec2 pos(map(key_frame.frame, s.left_frame, s_right_frame, left, right),
               map(key_frame.value, s.bottom_value, s.top_value, bottom, top));

    float x_diff = mouse_pos.x - pos.x;
    float y_diff = mouse_pos.y - pos.y;
    bool hovered = (x_diff * x_diff + y_diff * y_diff) < pow(5, 2);

    dl->AddCircleFilled(pos, 5, hovered ? 0xFF55CCFF : 0xFFFFFFFF);

    if (hovered)
      hovered_keyframe = i;
  }

  if (is_hovered && hovered_keyframe == -1) {
    ImVec2 pos(mouse_pos.x, map(mouse_x_curve_value, s.bottom_value,
                                s.top_value, bottom, top));

    dl->AddCircleFilled(pos, 4, 0x55FFFFFF);
  }

  float min_frame_x = map(min_frame, s.left_frame, s_right_frame, left, right);
  float max_frame_x = map(max_frame, s.left_frame, s_right_frame, left, right);

  if (min_frame_x > left)
    dl->AddRectFilled(ImVec2(left, top), ImVec2(min_frame_x, bottom),
                      0x99000000);

  if (max_frame_x < right)
    dl->AddRectFilled(ImVec2(max_frame_x, top), ImVec2(right, bottom),
                      0x99000000);

  dl->PopClipRect();

  if (is_hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    s.left_frame -= mouse_delta_frame;
    s.top_value -= mouse_delta_value;
    s.bottom_value -= mouse_delta_value;

    s_right_frame = s.left_frame + size.x / s.frame_width;

    if (s.left_frame > max_frame)
      s.left_frame = max_frame;
    else if (s_right_frame < min_frame)
      s.left_frame = min_frame - (s_right_frame - s.left_frame);
  }

  editor_states->insert_or_assign(id, s);
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

      int i = 0;

      auto& keyframe = track->at(i);

      ImGui::PushID(kfStringId.c_str());
      RSL_DEFER(ImGui::PopID());
      
      ImGui::TableSetColumnIndex(2);
	  
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      ImGui::Text("Keyframe %d", static_cast<int>(i));
      ImGui::SameLine();
      auto title = std::format(
          "{}##{}", (const char*)ICON_FA_TIMES_CIRCLE, kfStringId);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      if (ImGui::Button(title.c_str())) {
        track->erase(track->begin() + i);
      }
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      ImGui::InputFloat("Frame", &keyframe.frame);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      ImGui::InputFloat("Value", &keyframe.value);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      ImGui::InputFloat("Tangent", &keyframe.tangent);
   
	  ImGui::NewLine();
   	  
      float row_height = ImGui::GetCursorPosY() - top_of_row;
      ImGui::TableSetColumnIndex(3);
      auto size = ImGui::GetContentRegionAvail();
      size.y = row_height;
      
	  ShowSRTCurveEditor(kfStringId, size, track, frame_duration);

      //{
      //  ImGui::TableSetColumnIndex((track ? track->size() : 0) + 2);
      //  ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, matColor);
      //  auto title = std::format("{}##{}{}{}", "    +    ", matName, subtrack,
      //                           targetMtxId);
      //  auto avail = ImVec2{ImGui::GetContentRegionAvail().x, 0.0f};
      //  if (ImGui::Button(title.c_str(), avail)) {
      //    // Define a new keyframe with default values
      //    librii::g3d::SRT0KeyFrame newKeyframe;
      //    newKeyframe.frame = 0.0f;
      //    newKeyframe.value = 0.0f; // TODO: Grab last if exist
      //    newKeyframe.tangent = 0.0f;

      //    if (!track) {
      //      auto& mtx = anim.matrices.emplace_back();
      //      mtx.target = target;
      //      track = &(&mtx.matrix.scaleX)[subtrack];
      //    }
      //    track->push_back(newKeyframe);
      //  }
      //}
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
