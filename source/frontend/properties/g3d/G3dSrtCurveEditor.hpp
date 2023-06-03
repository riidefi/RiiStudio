#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/math/util.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

using namespace librii::g3d;
using librii::math::map;

namespace riistudio::g3d {

void srt_curve_editor(std::string id, ImVec2 size,
                      librii::g3d::SrtAnim::Track* track, float track_duration);

struct ControlPointPositions {
  ImVec2 keyframe;
  ImVec2 left;
  ImVec2 right;
  bool has_left() { return !isnan(left.x); }
  bool has_right() { return !isnan(right.x); }
};

class CurveEditor {
public:
  struct GuiFrameContext {
    ImDrawList* dl;
    ImRect viewport;
    librii::g3d::SrtAnim::Track* track;
    float track_duration;

    float left() { return viewport.Min.x; }
    float top() { return viewport.Min.y; }
    float right() { return viewport.Max.x; }
    float bottom() { return viewport.Max.y; }
  };

private:
  enum ControlPointPos {
    ControlPointPos_Left,
    ControlPointPos_Mid,
    ControlPointPos_Right,
  };

  struct HoveredPart {
  private:
    int m_type;
    SRT0KeyFrame m_key_frame;
    bool m_is_right_control_point;

  public:
    static HoveredPart None();
    bool is_None();

    static HoveredPart Keyframe(SRT0KeyFrame& keyframe);
    bool is_Keyframe(SRT0KeyFrame& keyframe);
    bool is_Keyframe();

    static HoveredPart ControlPoint(SRT0KeyFrame& keyframe, bool is_right);
    bool is_ControlPoint(SRT0KeyFrame& keyframe, bool& is_right);
    bool is_ControlPoint();

	static HoveredPart CurvePoint(float frame, float value);
    bool is_CurvePoint(float& frame, float& value);
    bool is_CurvePoint();

    bool is_keyframe_part(SRT0KeyFrame& keyframe,
                          ControlPointPos& control_point);

    bool is_keyframe_part();

    operator bool() { return !is_None(); }
    void operator|=(HoveredPart other) {
      if (other)
        *this = other;
    }
  };

  struct EditorView {
    float left_frame;
    float top_value;
    float bottom_value;
    float frame_width;

    float right_frame(ImRect viewport) {
      return left_frame + viewport.GetWidth() / frame_width;
    }
  };

  static std::map<std::string, CurveEditor> s_curve_editors;

  HoveredPart m_dragged_item;
  EditorView m_view;
  std::string m_id;
  short m_imgui_id;

  inline float left_frame() { return m_view.left_frame; }
  inline float right_frame(GuiFrameContext& c) {
    return m_view.right_frame(c.viewport);
  }
  inline float top_value() { return m_view.top_value; }
  inline float bottom_value() { return m_view.bottom_value; }

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

  void handle_dragging_keyframe(GuiFrameContext& c, SRT0KeyFrame& keyframe,
                                ControlPointPos control_point,
                                ControlPointPositions* pos_array);
  void handle_zooming(GuiFrameContext& c);
  void handle_panning(GuiFrameContext& c);
  void handle_auto_scrolling(GuiFrameContext& c);
  void keep_anim_area_in_view(GuiFrameContext& c);

  ImVec2 calc_tangent_intersection(GuiFrameContext& c, SRT0KeyFrame& keyframe,
                                   float intersection_frame);

  void calc_control_point_positions(GuiFrameContext& c,
                                    ControlPointPositions* dest_array,
                                    int maxsize);

  HoveredPart hit_test_keyframe(GuiFrameContext& c, SRT0KeyFrame& keyframe,
                                ControlPointPositions& positions);

  void draw_curve(GuiFrameContext& c, ControlPointPositions* pos_array);
  float sample_curve(GuiFrameContext& c, float frame);

  void draw_keyframe(GuiFrameContext& c, SRT0KeyFrame& keyframe,
                     ControlPointPositions& positions,
                     HoveredPart& hovered_part);

  void draw_track_bounds(GuiFrameContext& c);

public:
  void exe_gui(GuiFrameContext& c);

  static void update_editor_state(CurveEditor& edit) {
    s_curve_editors.insert_or_assign(edit.m_id, edit);
  }
  static CurveEditor get_or_init(std::string id, GuiFrameContext& c);

  CurveEditor(const EditorView& m_view, std::string id) : m_view(m_view) {
    m_dragged_item = HoveredPart::None();
    m_id = id;
    m_imgui_id = ImGui::GetID(id.c_str());
  };
};

} // namespace riistudio::g3d
