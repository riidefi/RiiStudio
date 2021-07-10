#include "StudioWindow.hpp"

namespace riistudio::frontend {

StudioWindow::StudioWindow(const std::string& name, bool dockspace)
    : mName(name), mbDrawDockspace(dockspace) {
  mId = ImGui::GetID(name.c_str()); // TODO
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

std::string StudioWindow::idIfy(std::string english_title_of_self) {
  std::string win_title = english_title_of_self;
  std::string win_parent_title = "";

  // TODO: Deeper layers?
  if (getParent() != nullptr &&
      dynamic_cast<StudioWindow*>(getParent()) != nullptr) {
    win_parent_title = reinterpret_cast<StudioWindow*>(getParent())->mName;
  }

  std::string internal_id = GetInternalIDOfWindow(win_title, win_parent_title);

  return TagWindowNameWithInternalID(
      riistudio::translateString(english_title_of_self), internal_id);
}
std::string StudioWindow::idIfyChild(std::string title) {
  return TagWindowNameWithInternalID(title,
                                     GetInternalIDOfWindow(title, mName));
}
void StudioWindow::draw() {
  // Inherit window class (only can dock children into a parent)
  StudioWindow* parent = dynamic_cast<StudioWindow*>(getParent());
  if (parent)
    ImGui::SetNextWindowClass(parent->getWindowClass());

  // Set window size to 1/4th of parent region
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextWindowSize(ImVec2{avail.x / 2, avail.y / 2}, ImGuiCond_Once);

  // The window name serves as the ID, use ### to ignore translated component
  const auto id_name = idIfy(mName);
  if (ImGui::Begin(id_name.c_str(), getOpen(), mFlags)) {
    ImGui::PushID(mName.c_str() /*mId*/);

    if (mbDrawDockspace) {
      const ImGuiID dockspaceId =
          ImGui::GetID(idIfyChild("###DockSpace").c_str());
      if (!ImGui::DockBuilderGetNode(dockspaceId)) {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);

        buildDock(dockspaceId);

        ImGui::DockBuilderFinish(dockspaceId);
      }
      ImGui::DockSpace(dockspaceId, {}, ImGuiDockNodeFlags_CentralNode,
                       getWindowClass());
    }
    draw_();
    drawChildren();

    // TODO
    //  if (ImGui::IsItemActive())
    //  	pWin->setActiveWindow(this);

    ImGui::PopID();
  }
  ImGui::End();
}
} // namespace riistudio::frontend
