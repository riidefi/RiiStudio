#include "studio_window.hpp"

namespace riistudio::frontend {

StudioWindow::StudioWindow(const std::string& name, bool dockspace)
    : mName(name), mbDrawDockspace(dockspace) {
  mId = ImGui::GetID(name.c_str()); // TODO
  mWindowClass.DockingAllowUnclassed = false;
  mWindowClass.ClassId = mId;
}

std::string StudioWindow::idIfy(std::string in) {
  std::string out = in + "##";
  if (getParent() != nullptr &&
      dynamic_cast<StudioWindow*>(getParent()) != nullptr) {
    out += reinterpret_cast<StudioWindow*>(getParent())->mName;
  }
  return out;
}
std::string StudioWindow::idIfyChild(std::string title) {
  return title + "##" + mName;
}
void StudioWindow::draw() {
  StudioWindow* parent = dynamic_cast<StudioWindow*>(getParent());
  if (parent)
    ImGui::SetNextWindowClass(parent->getWindowClass());
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextWindowSize(ImVec2{avail.x / 2, avail.y / 2}, ImGuiCond_Once);
  if (ImGui::Begin(idIfy(mName).c_str(), getOpen(), mFlags)) {
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
