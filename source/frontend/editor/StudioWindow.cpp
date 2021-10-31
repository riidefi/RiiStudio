#include "StudioWindow.hpp"

namespace riistudio::frontend {

StudioWindow::StudioWindow(const std::string& name, bool dockspace)
    : mbDrawDockspace(dockspace) {
  setName(name);
  mWindowClass.DockingAllowUnclassed = false;
  mWindowClass.ClassId = mId;
}

std::string GetInternalIDOfWindow(std::string win_title,
                                  std::string win_parent_title = "") {
  return win_title + " of " + win_parent_title;
}

std::string TagWindowNameWithInternalID(std::string win_title,
                                        std::string internal_id) {
  return win_title + "###" + internal_id;
}

std::string StudioWindow::idIfy(std::string english_title_of_self,
                                frontend::IWindow* parent) {
  std::string win_title = english_title_of_self;
  std::string win_parent_title = "";

  // TODO: Deeper layers?
  if (parent != nullptr && dynamic_cast<StudioWindow*>(parent) != nullptr) {
    win_parent_title = reinterpret_cast<StudioWindow*>(parent)->mName;
  }

  std::string internal_id = GetInternalIDOfWindow(win_title, win_parent_title);

  return TagWindowNameWithInternalID(
      riistudio::translateString(english_title_of_self), internal_id);
}
std::string StudioWindow::idIfyChild(std::string title) {
  return TagWindowNameWithInternalID(title,
                                     GetInternalIDOfWindow(title, mName));
}
void StudioWindow::drawDockspace() {
  const ImGuiID dockspaceId = ImGui::GetID(idIfyChild("###DockSpace").c_str());
  if (!ImGui::DockBuilderGetNode(dockspaceId)) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);

    buildDock(dockspaceId);

    ImGui::DockBuilderFinish(dockspaceId);
  }
  ImGui::DockSpace(dockspaceId, {}, ImGuiDockNodeFlags_CentralNode,
                   getWindowClass());
}

void StudioWindow::draw() {
  if (Begin(mName, getOpen(), mFlags, getParent())) {
    if (mbDrawDockspace) {
      drawDockspace();
    }
    draw_();
    drawChildren();
  }
  End();
}
} // namespace riistudio::frontend
