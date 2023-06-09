#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/math/util.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

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

  bool is_selected(KeyframeIndex item) { return m_selected.contains(item); }

  bool is_active(KeyframeIndex item) { return item == m_active; }

  std::optional<KeyframeIndex> get_active() { return m_active; }

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
                      librii::g3d::SrtAnim::Track* track, float track_duration,
                      KeyframeIndexSelection* keyframe_selection = nullptr);

//! @brief Stores all the external state necessary for using the CurveEditor
struct GuiFrameContext {
  ImDrawList* dl;
  ImRect viewport;
  librii::g3d::SrtAnim::Track* track;
  float track_duration = -1;
  KeyframeIndexSelection* keyframe_selection;

  float left() { return viewport.Min.x; }
  float top() { return viewport.Min.y; }
  float right() { return viewport.Max.x; }
  float bottom() { return viewport.Max.y; }
};

//! @brief Represents and stores the editors "camera" bounds, for zooming,
//! panning etc.
struct EditorView {
  float left_frame;
  float top_value;
  float bottom_value;
  float frame_width;

  float right_frame(ImRect viewport) {
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
  };

private:
  enum ControlPointPos {
    Left,
    Mid,
    Right,
  };

  //! @brief Stores the calculated control point positions in screen space for
  //! each keyframe
  struct ControlPointPositions {
    ImVec2 keyframe;
    std::optional<ImVec2> left = std::nullopt;
    std::optional<ImVec2> right = std::nullopt;
  };

  //! @brief A union/varient type for all kinds of Hoverable and or Draggable
  //! parts
  struct HoveredPart {
  private:
    using _None = std::monostate;

    struct _ControlPoint {
      KeyframeIndex keyframe = -1;
      bool is_right = false;

      _ControlPoint(KeyframeIndex keyframe, bool is_right)
          : keyframe(keyframe), is_right(is_right) {
      }
    };

    struct _CurvePoint {
      float frame;
      float value;

      _CurvePoint(float frame, float value) : frame(frame), value(value) {}
    };

    std::variant<_None, KeyframeIndex, _ControlPoint, _CurvePoint> m_part = _None();

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

    bool operator<(KeyframeState& other) {
      return keyframe.frame < other.keyframe.frame;
    }
  };

  HoveredPart m_dragged_item = HoveredPart::None();
  EditorView m_view;
  std::string m_id;
  short m_imgui_id = -1;

  float left_frame() { return m_view.left_frame; }
  float right_frame(GuiFrameContext& c) {
    return m_view.right_frame(c.viewport);
  }
  float top_value() { return m_view.top_value; }
  float bottom_value() { return m_view.bottom_value; }

#define map librii::math::map
  float frame_at(GuiFrameContext& c, float x) {
    return map(x, c.left(), c.right(), left_frame(), right_frame(c));
  }

  float value_at(GuiFrameContext& c, float y) {
    return map(y, c.top(), c.bottom(), top_value(), bottom_value());
  }

  float x_of_frame(GuiFrameContext& c, float frame) {
    return map(frame, left_frame(), right_frame(c), c.left(), c.right());
  }

  float y_of_value(GuiFrameContext& c, float value) {
    return map(value, top_value(), bottom_value(), c.top(), c.bottom());
  }
#undef map

  void handle_dragging_keyframe(GuiFrameContext& c,
                                ControlPointPositions* pos_array);

  void end_dragging_keyframe(GuiFrameContext& c);

  void delete_keyframe(GuiFrameContext& c, KeyframeIndex idx);
  void sort_keyframes(GuiFrameContext& c);

  void handle_zooming(GuiFrameContext& c);
  void handle_panning(GuiFrameContext& c);
  void handle_auto_scrolling(GuiFrameContext& c);
  void keep_anim_area_in_view(GuiFrameContext& c);

  void create_keyframe_state_vector(GuiFrameContext& c,
                                    std::vector<KeyframeState>& out);
  void update_from_keyframe_state_vector(GuiFrameContext& c,
                                         std::vector<KeyframeState>& v);

  bool is_keyframe_dragged(GuiFrameContext& c, KeyframeIndex keyframe) {
    KeyframeIndex dragged_keyframe;

    if (!m_dragged_item.is_Keyframe(dragged_keyframe))
      return false;

    return keyframe == dragged_keyframe ||
           (c.keyframe_selection->is_selected(dragged_keyframe) &&
            c.keyframe_selection->is_selected(keyframe));
  }

  ImVec2 calc_tangent_intersection(GuiFrameContext& c,
                                   librii::g3d::SRT0KeyFrame& keyframe,
                                   float intersection_frame);

  void calc_control_point_positions(GuiFrameContext& c,
                                    ControlPointPositions* dest_array,
                                    int maxsize);

  HoveredPart hit_test_keyframe(GuiFrameContext& c, KeyframeIndex keyframe,
                                ControlPointPositions& positions);

  void draw_curve(GuiFrameContext& c, ControlPointPositions* pos_array);
  float sample_curve(GuiFrameContext& c, float frame);

  void draw_keyframe(GuiFrameContext& c, KeyframeIndex keyframe,
                     ControlPointPositions& positions,
                     HoveredPart& hovered_part);

  void draw_track_bounds(GuiFrameContext& c);
};

} // namespace riistudio::g3d
