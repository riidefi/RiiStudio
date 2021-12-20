#include "ViewCube.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // ImGui::GetContentRegionMaxAbs()
#include <vendor/ImGuizmo.h>

namespace riistudio::lvl {

//
// TODO: ViewCube does not appear in non-docked view
//

bool DrawViewCube(float last_width, float last_height, glm::mat4& view_mtx,
                  u32 viewcube_bg) {
  /*
  Alt viewcube BG:
   * ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_WindowBg])
  */

  auto pos = ImGui::GetCursorScreenPos();
  // auto win = ImGui::GetWindowPos();
  auto avail = ImGui::GetContentRegionAvail();

  // int shifted_x = mViewport.mLastWidth - avail.x;
  int shifted_y = last_height - avail.y;

  // TODO: LastWidth is moved to the right, not left -- bug?
  ImGuizmo::SetRect(pos.x, pos.y - shifted_y, last_width, last_height);

  auto tVm = view_mtx;
  auto max = ImGui::GetContentRegionMaxAbs();

  ImGuizmo::ViewManipulate(glm::value_ptr(view_mtx), 20.0f,
                           {max.x - 200.0f, max.y - 200.0f}, {200.0f, 200.0f},
                           viewcube_bg);

  return tVm != view_mtx;
}

} // namespace riistudio::lvl
