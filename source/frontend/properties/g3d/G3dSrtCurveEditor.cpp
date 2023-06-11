#include "G3dSrtCurveEditor.hpp"

const ImU32 CurveColor = 0xFFCC33FF;
const float CurveHitTestThickness = 8;
const float CurveLineThickness = 1.5;
const float CurvePointRadius = 3.5;

const ImU32 HoverColor = 0xFF55CCFF;

const ImU32 KeyframeColor = 0xFFFFFFFF;
const float KeyframePointRadius = 5;
const float ControlPointRadius = 3;
const ImU32 KeyframeActiveOutlineColor = 0xFFFFFF33;
const ImU32 KeyframeSelectionOutlineColor = 0xFFFF7733;
const float KeyframeSelectionOutlineThickness = 2;

const ImU32 CursorTextColor = 0xFFFFFFFF;
const ImVec2 CursorTextOffset = ImVec2(15, 0);

const ImU32 TrackBoundsColor = 0x99000000;

const ImU32 ZeroValueLineColor = 0x66FFFFFF;
const float ZeroValueLineThickness = 0.5;

const float EditView_DefaultTopValue = 1;
const float EditView_DefaultBottomValue = 0;
const float EditView_ZoomFactor = 1.05;
const float EditView_ZoomFactorAutoScrollDeltaFactor = 0.2;

const float MouseDragThreshold = 5;

using namespace librii::g3d;
using librii::math::hermite;
using librii::math::map;

namespace riistudio::g3d {

std::unordered_map<std::string, CurveEditor> global_curve_editors =
    std::unordered_map<std::string, CurveEditor>();

void srt_curve_editor(std::string id, ImVec2 size,
                      librii::g3d::SrtAnim::Track* track, float track_duration,
                      KeyframeIndexSelection* keyframe_selection) {

  GuiFrameContext ctx;
  ctx.dl = ImGui::GetWindowDrawList();
  ctx.viewport.Min = ImGui::GetCursorScreenPos();
  ctx.viewport.Max = ctx.viewport.Min + size;
  ctx.track_duration = track_duration;
  ctx.track = track;
  ctx.keyframe_selection = keyframe_selection;

  CurveEditor editor;

  auto found = global_curve_editors.find(id);
  if (found != global_curve_editors.end())
    editor = global_curve_editors.at(found->first);
  else
    editor = CurveEditor::create_and_init(id, ctx);

  editor.exe_gui(ctx);

  global_curve_editors.insert_or_assign(id, editor);
}

#pragma region CurveEditor impl

CurveEditor CurveEditor::create_and_init(std::string id, GuiFrameContext& c) {
  // initialize curve editor state
  EditorView edit_view;
  edit_view.left_frame = 0;
  edit_view.top_value = EditView_DefaultTopValue;
  edit_view.bottom_value = EditView_DefaultBottomValue;
  edit_view.frame_width = c.viewport.GetWidth() / c.track_duration;

  auto edit = CurveEditor(edit_view, id);

  // make sure to include all keyframes
  // in the initial view
  std::vector<ControlPointPositions> pos_array(c.track->size());
  edit.calc_control_point_positions(c, pos_array, c.track->size());

  for (int i = 0; i < c.track->size(); i++) {
    auto& keyframe = c.track->at(i);
    auto& positions = pos_array[i];

    // keyframe
    {
      float value = keyframe.value;
      edit_view.bottom_value = std::min(edit_view.bottom_value, value);
      edit_view.top_value = std::max(edit_view.top_value, value);
    }

    // left control point
    if (positions.left) {
      float value = edit.value_at(c, positions.left->y);

      edit_view.bottom_value = std::min(edit_view.bottom_value, value);
      edit_view.top_value = std::max(edit_view.top_value, value);
    }

    // right control point
    if (positions.right) {
      float value = edit.value_at(c, positions.right->y);

      edit_view.bottom_value = std::min(edit_view.bottom_value, value);
      edit_view.top_value = std::max(edit_view.top_value, value);
    }
  }
  float width = c.viewport.GetWidth();
  float height = c.viewport.GetHeight();
  const float padding = KeyframePointRadius * 2;

  edit_view.top_value = map(0, padding, height - 2 * padding,
                            edit_view.top_value, edit_view.bottom_value);

  edit_view.bottom_value = map(height, padding, height - 2 * padding,
                               edit_view.top_value, edit_view.bottom_value);

  edit_view.left_frame =
      map(0, padding, width - 2 * padding, edit_view.left_frame,
          edit_view.right_frame(c.viewport));

  float _right_frame =
      map(width, padding, width - 2 * padding, edit_view.left_frame,
          edit_view.right_frame(c.viewport));

  edit_view.frame_width = width / (_right_frame - edit_view.left_frame);

  edit.m_view = edit_view;

  return edit;
}

void CurveEditor::exe_gui(GuiFrameContext& c) {
  // REMARK: when modifying the c.track other than push_back or assigning an
  // element you MUST use create_keyframe_state_vector with
  // update_from_keyframe_state_vector and modify the state vector in between
  // instead of c.track to ensure that the editor state and context is still
  // valid afterwards. this includes sorting as well!

#pragma region ImGui widget boilerplate

  if (!ImGui::ItemAdd(c.viewport, m_imgui_id))
    return;

  ImGui::ItemHoverable(c.viewport, m_imgui_id);

  auto window = ImGui::GetCurrentWindow();
  bool is_widget_hovered = ImGui::IsItemHovered();

  // make sure dragging out of the widget works as expected
  if (is_widget_hovered && ImGui::GetIO().MouseClicked[ImGuiMouseButton_Left]) {
    ImGui::SetActiveID(m_imgui_id, window);
    ImGui::SetFocusID(m_imgui_id, window);
    ImGui::FocusWindow(window);
  }

  if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
    is_widget_hovered = ImGui::GetActiveID() == m_imgui_id;
  }

  if (ImGui::GetActiveID() == m_imgui_id &&
      ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left]) {
    ImGui::ClearActiveID();
  }

#pragma endregion

  // used to store the calculated keyframe and control point positions in screen
  // space so they can be calculated once and looked up afterwards
  std::vector<ControlPointPositions> pos_array;
  bool is_pos_array_up_to_date = false;

  // handle dragging

  float frame, value;
  if (m_dragged_item.is_CurvePoint(frame, value)) {
    // Create and add new keyframe and set it as dragged
    SRT0KeyFrame keyframe;
    keyframe.frame = frame;
    keyframe.value = value;
    keyframe.tangent = 0;
    c.track->push_back(keyframe);
    m_dragged_item = HoveredPart::Keyframe(c.track->size() - 1);
  }

  pos_array = std::vector<ControlPointPositions>(c.track->size());

  if (m_dragged_item.is_keyframe_part() &&
      ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left,
                                      MouseDragThreshold)) {
    calc_control_point_positions(c, pos_array, c.track->size());
    is_pos_array_up_to_date = true;
    handle_dragging_keyframe_part(c, pos_array);
  }

  sort_keyframes(c);

  if (is_widget_hovered) {
    // camera controls
    handle_zooming(c);

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
        m_dragged_item.is_None())
      handle_panning(c);

    keep_anim_area_in_view(c);

    // camera changes need to be taken into account
    is_pos_array_up_to_date = false;
  }

  if (!is_pos_array_up_to_date) /*can have various reasons*/
    calc_control_point_positions(c, pos_array, c.track->size());

  auto hovered_part = HoveredPart::None();
  if (is_widget_hovered)
    hovered_part = determine_hovered_part(c, ImGui::GetMousePos(), pos_array);

  // Mouse Down/Up

  if (!m_dragged_item && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    m_dragged_item = hovered_part;

  else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    if (is_widget_hovered && !ImGui::IsMouseDragPastThreshold(
                                 ImGuiMouseButton_Left, MouseDragThreshold)) {
      handle_clicking_on(c, hovered_part);
    } else if (m_dragged_item.is_Keyframe()) {
      end_dragging_keyframe(c);
    }

    m_dragged_item = HoveredPart::None();
  }

  // all changes have been made, now we can finally draw!
  draw(c, is_widget_hovered, hovered_part, pos_array);
}

void CurveEditor::handle_clicking_on(GuiFrameContext& c,
                                     HoveredPart& hovered_part) {
  KeyframeIndex keyframe;

  if (c.keyframe_selection && m_dragged_item.is_None() &&
      ImGui::GetIO().KeyMods == ImGuiModFlags_None) {
    c.keyframe_selection->clear();
  } else if (c.keyframe_selection && m_dragged_item.is_Keyframe(keyframe)) {
    if (ImGui::GetIO().KeyMods == ImGuiModFlags_None) {
      bool should_select = !c.keyframe_selection->is_active(keyframe);

      c.keyframe_selection->clear();

      if (should_select)
        c.keyframe_selection->select(keyframe, true);
    } else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl) {
      if (c.keyframe_selection->is_active(keyframe)) {
        c.keyframe_selection->deselect(keyframe);
      } else {
        c.keyframe_selection->select(keyframe, true);
      }
    }
  }
}

void CurveEditor::handle_dragging_keyframe_part(
    GuiFrameContext& c, std::span<ControlPointPositions> pos_array) {
  KeyframeIndex dragged_keyframe;
  ControlPointPos dragged_control_point;
  bool ok =
      m_dragged_item.is_keyframe_part(dragged_keyframe, dragged_control_point);
  assert(ok);

  handle_auto_scrolling(c);

  int hovered_part_index = -1;
  auto hovered_part = HoveredPart::None();

  for (int i = 0; i < c.track->size(); i++) {
    if (is_keyframe_dragged(c, i) ||
        i == dragged_keyframe /*when control point is dragged*/)
      continue;

    auto _hovered_part = hit_test_keyframe(c, i, pos_array[i]);
    if (_hovered_part)
      hovered_part_index = i;

    hovered_part |= _hovered_part;
  }

  auto mouse_pos = ImGui::GetMousePos();

  KeyframeIndex hovered_keyframe;
  bool is_right;
  if (hovered_part.is_Keyframe()) {
    mouse_pos = pos_array[hovered_part_index].keyframe;

  } else if (dragged_control_point != ControlPointPos::Mid &&
             hovered_part.is_ControlPoint(hovered_keyframe, is_right)) {
    auto point_positions = pos_array[hovered_part_index];

    if (is_right) {
      assert(point_positions.right);
      mouse_pos = *point_positions.right;
    } else {
      assert(point_positions.left);
      mouse_pos = *point_positions.left;
    }
  }

  if (m_dragged_item.is_Keyframe()) {
    // support multi keyframe drag
    float dragged_keyframe_frame = c.track->at(dragged_keyframe).frame;
    float dragged_keyframe_value = c.track->at(dragged_keyframe).value;

    for (int i = 0; i < c.track->size(); i++) {

      auto keyframe = c.track->at(i);

      if (!is_keyframe_dragged(c, i))
        continue;

      float frame_offset = keyframe.frame - dragged_keyframe_frame;
      float value_offset = keyframe.value - dragged_keyframe_value;

      KeyframeIndex hovered_keyframe;
      if (hovered_part.is_Keyframe(hovered_keyframe) && i == dragged_keyframe) {
        /* little hack to ensure the keyframes get assigned the exact frame and
           value when snapping */
        keyframe.frame = c.track->at(hovered_keyframe).frame;
        keyframe.value = c.track->at(hovered_keyframe).value;

      } else {
        keyframe.frame = frame_at(c, mouse_pos.x) + frame_offset;
        keyframe.value = value_at(c, mouse_pos.y) + value_offset;
      }

      c.track->at(i) = keyframe;
    }
  } else if (m_dragged_item.is_ControlPoint()) {
    auto keyframe = c.track->at(dragged_keyframe);

    float frame = frame_at(c, mouse_pos.x);
    float value = value_at(c, mouse_pos.y);

    float new_tangent = (value - keyframe.value) / (frame - keyframe.frame);

    if (!std::isnan(new_tangent) && !std::isinf(new_tangent))
      keyframe.tangent = new_tangent;

    c.track->at(dragged_keyframe) = keyframe;
  }
}

void CurveEditor::end_dragging_keyframe(GuiFrameContext& c) {
  assert(m_dragged_item.is_Keyframe());

  auto dragged_keyframe_frames = std::set<float>();

  for (int i = 0; i < c.track->size(); i++) {
    if (is_keyframe_dragged(c, i)) {
      dragged_keyframe_frames.insert(c.track->at(i).frame);
    }
  }

  int count_before = c.track->size();

  auto vec = std::vector<KeyframeState>();
  create_keyframe_state_vector(c, vec);

  for (int i = c.track->size() - 1; i >= 0; i--) {
    auto k = c.track->at(i);
    if (!is_keyframe_dragged(c, i) && dragged_keyframe_frames.contains(k.frame))
      vec.erase(vec.begin() + i);
  }

  update_from_keyframe_state_vector(c, vec);

  std::cout << std::format("deleted: {} keyframes\n",
                           count_before - c.track->size());
}

void CurveEditor::handle_zooming(GuiFrameContext& c) {
  if (!ImGui::GetIO().KeyCtrl)
    return;

  float mouse_pos_frame = frame_at(c, ImGui::GetMousePos().x);
  float mouse_pos_value = value_at(c, ImGui::GetMousePos().y);
  float scale_factor = pow(EditView_ZoomFactor, -ImGui::GetIO().MouseWheel);

  bool zoom_horizontally = false;
  bool zoom_vertically = false;

  if (ImGui::GetIO().KeyMods == (ImGuiModFlags_Ctrl | ImGuiModFlags_Shift)) {
    zoom_horizontally = true;
  } else if (ImGui::GetIO().KeyMods ==
             (ImGuiModFlags_Ctrl | ImGuiModFlags_Alt)) {
    zoom_vertically = true;
  } else {
    zoom_horizontally = true;
    zoom_vertically = true;
  }

  if (zoom_horizontally) {
    m_view.left_frame =
        mouse_pos_frame + (left_frame() - mouse_pos_frame) * scale_factor;

    m_view.frame_width /= scale_factor;
  }
  if (zoom_vertically) {
    m_view.top_value =
        mouse_pos_value + (top_value() - mouse_pos_value) * scale_factor;
    m_view.bottom_value =
        mouse_pos_value + (bottom_value() - mouse_pos_value) * scale_factor;
  }
}

void CurveEditor::handle_panning(GuiFrameContext& c) {
  auto mouse_delta = ImGui::GetIO().MouseDelta;
  float mouse_delta_frame = mouse_delta.x / m_view.frame_width;
  float mouse_delta_value =
      mouse_delta.y * (bottom_value() - top_value()) / (c.bottom() - c.top());

  m_view.left_frame -= mouse_delta_frame;
  m_view.top_value -= mouse_delta_value;
  m_view.bottom_value -= mouse_delta_value;
}

void CurveEditor::handle_auto_scrolling(GuiFrameContext& c) {
  auto mouse_pos = ImGui::GetMousePos();

  float mouse_pos_frame = frame_at(c, mouse_pos.x);
  float mouse_pos_value = value_at(c, mouse_pos.y);

  float scrollXDeltaFrames = 0;
  float scrollYDeltaValue = 0;

  scrollXDeltaFrames += std::min(0.0f, mouse_pos_frame - left_frame());
  scrollXDeltaFrames += std::max(0.0f, mouse_pos_frame - right_frame(c));

  scrollYDeltaValue += std::max(0.0f, mouse_pos_value - top_value());
  scrollYDeltaValue += std::min(0.0f, mouse_pos_value - bottom_value());

  scrollXDeltaFrames *= EditView_ZoomFactorAutoScrollDeltaFactor;
  scrollYDeltaValue *= EditView_ZoomFactorAutoScrollDeltaFactor;

  m_view.left_frame += scrollXDeltaFrames;
  m_view.top_value += scrollYDeltaValue;
  m_view.bottom_value += scrollYDeltaValue;
}

void CurveEditor::keep_anim_area_in_view(GuiFrameContext& c) {
  float min_frame = 0;
  float max_frame = c.track_duration;

  if (left_frame() > max_frame)
    m_view.left_frame = max_frame;
  else if (right_frame(c) < min_frame)
    m_view.left_frame = min_frame - (right_frame(c) - left_frame());
}

void CurveEditor::sort_keyframes(GuiFrameContext& c) {
  auto by_frame = [](auto& a, auto& b) { return a.frame < b.frame; };
  if (std::is_sorted(c.track->begin(), c.track->end(), by_frame))
    return;

  auto vec = std::vector<KeyframeState>();
  create_keyframe_state_vector(c, vec);

  std::sort(vec.begin(), vec.end());

  update_from_keyframe_state_vector(c, vec);
}

void CurveEditor::create_keyframe_state_vector(
    GuiFrameContext& c, std::vector<CurveEditor::KeyframeState>& out) {
  out = std::vector<KeyframeState>(c.track->size());
  for (int i = 0; i < c.track->size(); i++) {
    out[i].keyframe = c.track->at(i);
    out[i].is_selected = c.keyframe_selection->is_selected(i);
    out[i].is_active = c.keyframe_selection->is_active(i);

    KeyframeIndex keyframe;
    out[i].is_dragged = (m_dragged_item.is_Keyframe(keyframe) && keyframe == i);
  }
}

void CurveEditor::update_from_keyframe_state_vector(
    GuiFrameContext& c, std::vector<CurveEditor::KeyframeState>& vec) {
  c.track->resize(vec.size());

  c.keyframe_selection->clear();

  if (m_dragged_item.is_Keyframe())
    m_dragged_item = HoveredPart::None();

  for (int i = 0; i < vec.size(); i++) {
    c.track->at(i) = vec[i].keyframe;
    if (vec[i].is_selected)
      c.keyframe_selection->select(i, vec[i].is_active);

    if (vec[i].is_dragged)
      m_dragged_item = HoveredPart::Keyframe(i);
  }
}

ImVec2 CurveEditor::calc_tangent_intersection(GuiFrameContext& c,
                                              SRT0KeyFrame& keyframe,
                                              float intersection_frame) {
  return ImVec2(
      x_of_frame(c, intersection_frame),
      y_of_value(c, keyframe.value + (intersection_frame - keyframe.frame) *
                                         keyframe.tangent));
}

void CurveEditor::calc_control_point_positions(
    GuiFrameContext& c, std::span<ControlPointPositions> dest_array,
    int max_size) {
  using Optional = std::optional<SRT0KeyFrame>;

  struct OverlapGroup {
    float frame;
    float value;
    Optional resolve_keyframe;
    int begin;
    Optional left;
  };

  auto calc = [this, &c](Optional left, SRT0KeyFrame mid, Optional right) {
    ControlPointPositions cpp;
    cpp.left = ImVec2(0 / 0.0, 0 / 0.0);
    cpp.right = ImVec2(0 / 0.0, 0 / 0.0);

    cpp.keyframe = ImVec2(x_of_frame(c, mid.frame), y_of_value(c, mid.value));

    if (left) {
      cpp.left =
          calc_tangent_intersection(c, mid, (mid.frame + left->frame) / 2);
    }

    if (right) {
      cpp.right =
          calc_tangent_intersection(c, mid, (mid.frame + right->frame) / 2);
    }

    return cpp;
  };

  if (c.track->empty())
    return;

  OverlapGroup overlap_group;
  overlap_group.frame = c.track->at(0).frame;
  overlap_group.value = c.track->at(0).value;
  overlap_group.resolve_keyframe = std::nullopt;
  overlap_group.begin = 0;
  overlap_group.left = std::nullopt;

  Optional left = std::nullopt;
  Optional mid = std::nullopt;
  Optional right = Optional(c.track->at(0));

  for (int i = 1; i < c.track->size() + 1; i++) {
    int mid_keyframe_idx = i - 1;
    int right_keyframe_idx = i;

    left = mid;
    mid = right;
    right = std::nullopt;

    if (i < c.track->size())
      right = c.track->at(i);

    assert(mid);

    auto cpp = calc(left, *mid, right);

    assert(mid_keyframe_idx < max_size);

    dest_array[mid_keyframe_idx] = cpp;

    // handle overlapping keyframes

    if (is_keyframe_dragged(c, mid_keyframe_idx)) {
      overlap_group.resolve_keyframe = mid;
    }

    if (!right || right->frame != overlap_group.frame ||
        right->value != overlap_group.value) {
      // overlap group endend

      if (overlap_group.resolve_keyframe) {
        // resolve overlap with data of resolve keyframe
        auto resolve_keyframe = *overlap_group.resolve_keyframe;
        for (int j = overlap_group.begin; j < right_keyframe_idx; j++) {

          // make sure that the resolve keyframe is valid for resovling
          assert(c.track->at(j).frame == resolve_keyframe.frame &&
                 c.track->at(j).value == resolve_keyframe.value);

          auto _mid = resolve_keyframe;

          dest_array[j] = calc(overlap_group.left, _mid, right);
        }
      }

      // begin new overlap group
      if (right) {
        overlap_group.begin = right_keyframe_idx;
        overlap_group.frame = right->frame;
        overlap_group.value = right->value;
        overlap_group.resolve_keyframe = std::nullopt;
        overlap_group.left = mid;
      }
    }
  }
}

CurveEditor::HoveredPart CurveEditor::determine_hovered_part(
    GuiFrameContext& c, ImVec2 hit_point,
    std::span<ControlPointPositions> pos_array) {
  HoveredPart hovered_part = HoveredPart::None();

  float hit_frame = frame_at(c, hit_point.x);

  float sampled_value = 0;

  if (!c.track->empty())
    sampled_value = sample_curve(c, hit_frame);

  if (abs(y_of_value(c, sampled_value) - hit_point.y) <=
      CurveHitTestThickness / 2)
    hovered_part = HoveredPart::CurvePoint(hit_frame, sampled_value);

  for (int i = 0; i < c.track->size(); i++) {
    hovered_part |= hit_test_keyframe(c, i, pos_array[i]);
  }

  return hovered_part;
}

CurveEditor::HoveredPart
CurveEditor::hit_test_keyframe(GuiFrameContext& c, KeyframeIndex keyframe,
                               ControlPointPositions& positions) {
  auto hit_test_circle = [](ImVec2 point, ImVec2 center, float radius) {
    float x = point.x - center.x;
    float y = point.y - center.y;
    return (x * x + y * y) < radius * radius;
  };

  auto mouse_pos = ImGui::GetMousePos();

  auto hovered_part = HoveredPart::None();

  if (positions.left &&
      hit_test_circle(mouse_pos, *positions.left, ControlPointRadius))
    hovered_part |= HoveredPart::ControlPoint(keyframe, false);

  if (positions.right &&
      hit_test_circle(mouse_pos, *positions.right, ControlPointRadius))
    hovered_part |= HoveredPart::ControlPoint(keyframe, true);

  if (hit_test_circle(mouse_pos, positions.keyframe, KeyframePointRadius))
    hovered_part |= HoveredPart::Keyframe(keyframe);

  return hovered_part;
}

float CurveEditor::sample_curve(GuiFrameContext& c, float frame) {
  assert(!c.track->empty());

  auto left = c.track->at(0);
  auto right = left;

  if (frame < left.frame)
    return left.value + (frame - left.frame) * left.tangent;

  for (int i = 1; i < c.track->size(); i++) {
    right = c.track->at(i);

    if (frame < left.frame || frame > right.frame) {
      left = right;
      continue;
    }

    return hermite(frame, left.frame, left.value, left.tangent, right.frame,
                   right.value, right.tangent);
  }

  return right.value + (frame - right.frame) * right.tangent;
}

void CurveEditor::draw(GuiFrameContext& c, bool is_widget_hovered,
                       HoveredPart& hovered_part,
                       std::span<ControlPointPositions> pos_array) {
  c.dl->PushClipRect(c.viewport.Min, c.viewport.Max, true);

  c.dl->AddRect(c.viewport.Min, c.viewport.Max, 0x99000000);

  float y = y_of_value(c, 0);
  c.dl->AddLine(ImVec2(c.left(), y), ImVec2(c.right(), y), ZeroValueLineColor,
                ZeroValueLineThickness);

  {
    float frame, value;
    if (m_dragged_item.is_CurvePoint(frame, value) ||
        hovered_part.is_CurvePoint(frame, value)) {

      c.dl->AddCircleFilled(ImVec2(x_of_frame(c, frame), y_of_value(c, value)),
                            CurvePointRadius, CurveColor);
    }
  }

  if (c.track->empty()) {
    // draw curve as a simple line with constant value 0
    c.dl->AddLine(ImVec2(c.left(), y_of_value(c, 0)),
                  ImVec2(c.right(), y_of_value(c, 0)), CurveColor,
                  CurveLineThickness);
  } else {
    draw_curve(c, pos_array);

    for (int i = 0; i < c.track->size(); i++) {
      draw_keyframe(c, i, pos_array[i], hovered_part);
    }
  }

  draw_track_bounds(c);

  if (is_widget_hovered) {
    draw_cursor_text(c);
  }

  c.dl->PopClipRect();
}

void CurveEditor::draw_curve(GuiFrameContext& c,
                             std::span<ControlPointPositions> pos_array) {
  assert(!c.track->empty());

  if (c.track->at(0).frame > left_frame()) {
    c.dl->AddLine(calc_tangent_intersection(c, c.track->at(0), left_frame()),
                  pos_array[0].keyframe, CurveColor, CurveLineThickness);
  }
  int last_idx = c.track->size() - 1;

  if (c.track->at(last_idx).frame < right_frame(c)) {
    c.dl->AddLine(
        calc_tangent_intersection(c, c.track->at(last_idx), right_frame(c)),
        pos_array[last_idx].keyframe, CurveColor, CurveLineThickness);
  }

  for (int i = 1; i < c.track->size(); i++) {
    auto left = pos_array[i - 1];
    auto right = pos_array[i];

    if (!left.right && !right.left)
      continue;

    c.dl->AddBezierCubic(
        left.keyframe, left.keyframe + (*left.right - left.keyframe) * 2 / 3.0,
        right.keyframe + (*right.left - right.keyframe) * 2 / 3.0,
        right.keyframe, CurveColor,
        CurveLineThickness); /* this is apperantly how the hermite
                  curves translate to bezier*/
  }
}

void CurveEditor::draw_keyframe(GuiFrameContext& c, KeyframeIndex keyframe,
                                ControlPointPositions& positions,
                                HoveredPart& hovered_part) {
  if (c.keyframe_selection && c.keyframe_selection->is_selected(keyframe)) {
    c.dl->AddCircleFilled(positions.keyframe,
                          KeyframePointRadius +
                              KeyframeSelectionOutlineThickness,
                          c.keyframe_selection->is_active(keyframe)
                              ? KeyframeActiveOutlineColor
                              : KeyframeSelectionOutlineColor);
  }

  if (positions.left) {
    c.dl->AddLine(positions.keyframe, *positions.left, KeyframeColor, 1.2);

    KeyframeIndex _keyframe;
    bool is_right;
    bool hovered = hovered_part.is_ControlPoint(_keyframe, is_right) &&
                   keyframe == _keyframe && !is_right;
    c.dl->AddCircleFilled(*positions.left, ControlPointRadius,
                          hovered ? HoverColor : KeyframeColor);
  }

  if (positions.right) {
    c.dl->AddLine(positions.keyframe, *positions.right, KeyframeColor, 1.2);

    KeyframeIndex _keyframe;
    bool is_right;
    bool hovered = hovered_part.is_ControlPoint(_keyframe, is_right) &&
                   keyframe == _keyframe && is_right;
    c.dl->AddCircleFilled(*positions.right, ControlPointRadius,
                          hovered ? HoverColor : KeyframeColor);
  }

  // draw keyframe
  KeyframeIndex _keyframe;
  bool hovered = hovered_part.is_Keyframe(_keyframe) && keyframe == _keyframe;
  c.dl->AddCircleFilled(positions.keyframe, KeyframePointRadius,
                        hovered ? HoverColor : KeyframeColor);
}

void CurveEditor::draw_track_bounds(GuiFrameContext& c) {
  float min_frame_x = map(0, left_frame(), right_frame(c), c.left(), c.right());
  float max_frame_x =
      map(c.track_duration, left_frame(), right_frame(c), c.left(), c.right());

  if (min_frame_x > c.left())
    c.dl->AddRectFilled(ImVec2(c.left(), c.top()),
                        ImVec2(min_frame_x, c.bottom()), TrackBoundsColor);

  if (max_frame_x < c.right())
    c.dl->AddRectFilled(ImVec2(max_frame_x, c.top()),
                        ImVec2(c.right(), c.bottom()), TrackBoundsColor);
}

void CurveEditor::draw_cursor_text(GuiFrameContext& c) {
  auto mouse_pos = ImGui::GetMousePos();
  float mouse_pos_frame = frame_at(c, mouse_pos.x);
  float mouse_pos_value = value_at(c, mouse_pos.y);

  auto text =
      std::format("frame: {}\nvalue: {:.2f}", mouse_pos_frame, mouse_pos_value);

  auto size = ImGui::CalcTextSize(text.c_str());

  auto pos = mouse_pos + CursorTextOffset;

  if (m_dragged_item) {
    pos.x = std::clamp(pos.x, c.left(), c.right() - size.x);
    pos.y = std::clamp(pos.y, c.top(), c.bottom() - size.y);
  }

  c.dl->AddText(pos, CursorTextColor, text.c_str());
}

#pragma endregion

#pragma region CurveEditor::HoveredPart impl

CurveEditor::HoveredPart CurveEditor::HoveredPart::None() {
  auto part = HoveredPart();
  return part;
}
bool CurveEditor::HoveredPart::is_None() {
  return std::holds_alternative<_None>(m_part);
}

CurveEditor::HoveredPart
CurveEditor::HoveredPart::Keyframe(KeyframeIndex keyframe) {
  auto part = HoveredPart();
  part.m_part = keyframe;
  return part;
}
bool CurveEditor::HoveredPart::is_Keyframe() {
  return std::holds_alternative<KeyframeIndex>(m_part);
}

bool CurveEditor::HoveredPart::is_Keyframe(KeyframeIndex& keyframe) {
  if (auto* kf = std::get_if<KeyframeIndex>(&m_part); kf != nullptr) {
    keyframe = *kf;
    return true;
  }
  return false;
}

CurveEditor::HoveredPart
CurveEditor::HoveredPart::ControlPoint(const KeyframeIndex keyframe,
                                       bool is_right) {
  auto part = HoveredPart();
  part.m_part = _ControlPoint(keyframe, is_right);
  return part;
}
bool CurveEditor::HoveredPart::is_ControlPoint() {
  return std::holds_alternative<_ControlPoint>(m_part);
}

bool CurveEditor::HoveredPart::is_ControlPoint(KeyframeIndex& keyframe,
                                               bool& is_right) {
  if (auto* cp = std::get_if<_ControlPoint>(&m_part); cp != nullptr) {
    keyframe = cp->keyframe;
    is_right = cp->is_right;
    return true;
  }
  return false;
}

bool CurveEditor::HoveredPart::is_keyframe_part() {
  return is_Keyframe() || is_ControlPoint();
}
bool CurveEditor::HoveredPart::is_keyframe_part(
    KeyframeIndex& keyframe, ControlPointPos& control_point) {
  KeyframeIndex kf;
  bool is_right;
  if (is_Keyframe(kf)) {
    keyframe = kf;
    control_point = ControlPointPos::Mid;
    return true;
  }
  if (is_ControlPoint(kf, is_right)) {
    keyframe = kf;
    control_point = is_right ? ControlPointPos::Right : ControlPointPos::Left;
    return true;
  }

  return false;
}

CurveEditor::HoveredPart CurveEditor::HoveredPart::CurvePoint(float frame,
                                                              float value) {
  auto part = HoveredPart();
  part.m_part = _CurvePoint(frame, value);
  return part;
}
bool CurveEditor::HoveredPart::is_CurvePoint() {
  return std::holds_alternative<_CurvePoint>(m_part);
}
bool CurveEditor::HoveredPart::is_CurvePoint(float& frame, float& value) {
  if (auto* cp = std::get_if<_CurvePoint>(&m_part); cp != nullptr) {
    frame = cp->frame;
    value = cp->value;
    return true;
  }
  return false;
}

#pragma endregion

} // namespace riistudio::g3d
