#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/math/util.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

struct EditableTrack {
  librii::g3d::SrtAnim::Track* track;
  std::string name;
  u32 color = 0xFF;

  constexpr auto& at(int idx) { return track->at(idx); }
  auto empty() { return track->empty(); }
  auto size() { return track->size(); }
};

using KeyframeIndex = int;

class KeyframeIndexSelection {
public:
  KeyframeIndexSelection() = default;

  void select(KeyframeIndex item, bool set_active = false) {
    m_selected.insert(item);
    if (set_active)
      m_active = item;
  }

  void deselect(KeyframeIndex item) {
    if (m_selected.contains(item))
      m_selected.erase(item);

    if (m_active == item)
      m_active = std::nullopt;
  }

  void clear() {
    m_selected.clear();
    m_active = std::nullopt;
  }

  bool is_selected(KeyframeIndex item) const {
    return m_selected.contains(item);
  }

  bool is_active(KeyframeIndex item) const { return item == m_active; }

  std::optional<KeyframeIndex> get_active() const { return m_active; }

private:
  std::unordered_set<KeyframeIndex> m_selected;
  std::optional<KeyframeIndex> m_active = std::nullopt;
};

//! @brief
//! A widget that lets you edit a Curve/Track/List of keyframes in a visual way,
//! similar to blender and other animation software
//! @param id The ImGui widget id
//! @param size The size of the widget
//! @param track The track to display and edit
//! @param track_duration Duration of the track in frames (for showing
//! correct bounds)
//! @param keyframe_selection [optional] A set of selected
//! Keyframe indices (more or less), that may be modified by the widget
void srt_curve_editor(std::string id, ImVec2 size,
                      std::span<EditableTrack> tracks, float track_duration,
                      int& active_track_idx,
                      KeyframeIndexSelection* keyframe_selection = nullptr);

//! @brief Stores all the external state necessary for using the CurveEditor
struct GuiFrameContext {
  ImDrawList* dl = nullptr;
  ImRect viewport{};
  std::span<EditableTrack> tracks = std::span<EditableTrack>();
  float track_duration = -1;
  int active_track_idx = 0;
  KeyframeIndexSelection* keyframe_selection = nullptr;

  float left() const { return viewport.Min.x; }
  float top() const { return viewport.Min.y; }
  float right() const { return viewport.Max.x; }
  float bottom() const { return viewport.Max.y; }

  bool has_active_track() {
    return 0 <= active_track_idx && active_track_idx <= tracks.size() - 1;
  }
  EditableTrack* active_track() {
    return has_active_track() ? &tracks[active_track_idx] : nullptr;
  }
};

//! @brief Represents and stores the editors "camera" bounds, for zooming,
//! panning etc.
struct EditorView {
  float left_frame{};
  float top_value{};
  float bottom_value{};
  float frame_width{};

  float right_frame(ImRect viewport) const {
    return left_frame + viewport.GetWidth() / frame_width;
  }
};

//! @brief The backing class for the srt_curve_editor widget
class CurveEditor {
public:
  CurveEditor() = default;
  void exe_gui(GuiFrameContext& c);

  static CurveEditor create_and_init(std::string id, GuiFrameContext& c);

  CurveEditor(const EditorView& m_view, std::string id) : m_view(m_view) {
    m_id = id;
    m_imgui_id = ImGui::GetID(id.c_str());
  }

private:
  enum class ControlPointPos {
    Left,
    Mid,
    Right,
  };

  //! @brief Stores the calculated control point positions in screen space for
  //! each keyframe
  struct ControlPointPositions {
    ImVec2 keyframe{};
    std::optional<ImVec2> left = std::nullopt;
    std::optional<ImVec2> right = std::nullopt;
  };

  // used to store the calculated keyframe and control point positions in screen
  // space so they can be calculated once and looked up afterwards
  using PosArray = std::vector<ControlPointPositions>;

  //! @brief A union/varient type for all kinds of Hoverable and or Draggable
  //! parts
  struct HoveredPart {
  private:
    using _None = std::monostate;

    struct _ControlPoint {
      KeyframeIndex keyframe = -1;
      bool is_right = false;

      _ControlPoint(KeyframeIndex keyframe, bool is_right)
          : keyframe(keyframe), is_right(is_right) {}
    };

    struct _CurvePoint {
      float frame;
      float value;

      _CurvePoint(float frame, float value) : frame(frame), value(value) {}
    };

    struct _Curve {
      int track_idx;

      _Curve(int track_idx) : track_idx(track_idx) {}
    };

    std::variant<_None, KeyframeIndex, _ControlPoint, _CurvePoint, _Curve>
        m_part = _None();

  public:
    static HoveredPart None();
    bool is_None();

    static HoveredPart Keyframe(KeyframeIndex keyframe);
    bool is_Keyframe(KeyframeIndex& keyframe);
    bool is_Keyframe();

    static HoveredPart ControlPoint(KeyframeIndex keyframe, bool is_right);
    bool is_ControlPoint(KeyframeIndex& keyframe, bool& is_right);
    bool is_ControlPoint();

    static HoveredPart CurvePoint(float frame, float value);
    bool is_CurvePoint(float& frame, float& value);
    bool is_CurvePoint();

    static HoveredPart Curve(int track_idx);
    bool is_Curve(int& track_idx);
    bool is_Curve();

    bool is_keyframe_part(KeyframeIndex& keyframe,
                          ControlPointPos& control_point);

    bool is_keyframe_part();

    operator bool() { return !is_None(); }
    void operator|=(HoveredPart other) {
      if (other)
        *this = other;
    }
  };

  //! @brief Stores redundant data about each keyframe, to ensure their state is
  //! preserved after an operation that changes their indices.
  struct KeyframeState {
    librii::g3d::SRT0KeyFrame keyframe;
    bool is_selected = false;
    bool is_active = false;
    bool is_dragged = false;

    bool operator<(const KeyframeState& other) const {
      return keyframe.frame < other.keyframe.frame;
    }
  };

  HoveredPart m_dragged_item = HoveredPart::None();
  EditorView m_view;
  std::string m_id;
  short m_imgui_id = -1;

  float left_frame() const { return m_view.left_frame; }
  float right_frame(GuiFrameContext& c) const {
    return m_view.right_frame(c.viewport);
  }
  float top_value() const { return m_view.top_value; }
  float bottom_value() const { return m_view.bottom_value; }

#define map librii::math::map
  float frame_at(GuiFrameContext& c, float x) const {
    return map(x, c.left(), c.right(), left_frame(), right_frame(c));
  }

  float value_at(GuiFrameContext& c, float y) const {
    return map(y, c.top(), c.bottom(), top_value(), bottom_value());
  }

  float x_of_frame(GuiFrameContext& c, float frame) const {
    return map(frame, left_frame(), right_frame(c), c.left(), c.right());
  }

  float y_of_value(GuiFrameContext& c, float value) const {
    return map(value, top_value(), bottom_value(), c.top(), c.bottom());
  }
#undef map
  void handle_clicking_on(GuiFrameContext& c, HoveredPart& hovered_part);
  void handle_dragging_keyframe_part(GuiFrameContext& c,
                                     PosArray& active_track_pos_array);
  bool is_keyframe_dragged(GuiFrameContext& c, KeyframeIndex keyframe) {
    KeyframeIndex dragged_keyframe;

    if (!m_dragged_item.is_Keyframe(dragged_keyframe))
      return false;

    return keyframe == dragged_keyframe ||
           (c.keyframe_selection->is_selected(dragged_keyframe) &&
            c.keyframe_selection->is_selected(keyframe));
  }
  void end_dragging_keyframe(GuiFrameContext& c);

  void handle_zooming(GuiFrameContext& c);
  void handle_panning(GuiFrameContext& c);
  void handle_auto_scrolling(GuiFrameContext& c);
  void keep_anim_area_in_view(GuiFrameContext& c);

  void sort_keyframes(GuiFrameContext& c);
  void create_keyframe_state_vector(GuiFrameContext& c,
                                    std::vector<KeyframeState>& out);
  void update_from_keyframe_state_vector(GuiFrameContext& c,
                                         std::vector<KeyframeState>& v);

  ImVec2 calc_tangent_intersection(GuiFrameContext& c,
                                   librii::g3d::SRT0KeyFrame& keyframe,
                                   float intersection_frame);

  void calc_control_point_positions(GuiFrameContext& c,
                                    librii::g3d::SrtAnim::Track* track,
                                    PosArray& dest_array);

  HoveredPart determine_hovered_part(GuiFrameContext& c, ImVec2 hit_point,
                                     std::span<PosArray> pos_arrays);
  HoveredPart hit_test_keyframe(GuiFrameContext& c, KeyframeIndex keyframe,
                                ControlPointPositions& positions);
  float sample_curve(GuiFrameContext& c, librii::g3d::SrtAnim::Track* track,
                     float frame);

  void draw(GuiFrameContext& c, bool is_widget_hovered,
            HoveredPart& hovered_part, std::span<PosArray> pos_arrays);
  void draw_grid_lines(GuiFrameContext& c, bool is_vertical,
                       float min_minor_tick_size, int major_tick_interval);
  void draw_curve(GuiFrameContext& c, librii::g3d::SrtAnim::Track* track,
                  PosArray& pos_array, ImU32 color);
  void draw_keyframe(GuiFrameContext& c, KeyframeIndex keyframe,
                     ControlPointPositions& positions,
                     HoveredPart& hovered_part);
  void draw_track_bounds(GuiFrameContext& c);
  void draw_cursor_text(GuiFrameContext& c, HoveredPart& hovered_part);
};

} // namespace riistudio::g3d
