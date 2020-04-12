#include "studio_window.hpp"

namespace riistudio::frontend {

StudioWindow::StudioWindow(const std::string& name, bool dockspace)
    : mName(name), mbDrawDockspace(dockspace) {
  mId = ImGui::GetID(name.c_str()); // TODO
  mWindowClass.DockingAllowUnclassed = false;
  mWindowClass.ClassId = mId;
}
void StudioWindow::setWindowFlag(u32 flag) { mFlags |= flag; }

std::string StudioWindow::getName() const { return mName; }

const ImGuiWindowClass* StudioWindow::getWindowClass() { return &mWindowClass; }

std::string StudioWindow::idIfy(std::string in) {
  std::string out = in + "##";
  if (mParent != nullptr && dynamic_cast<StudioWindow*>(mParent) != nullptr) {
    out += reinterpret_cast<StudioWindow*>(mParent)->mName;
  }
  return out;
}
std::string StudioWindow::idIfyChild(std::string title) {
  return title + "##" + mName;
}
void StudioWindow::draw() {
  StudioWindow* parent = dynamic_cast<StudioWindow*>(mParent);
  if (parent)
    ImGui::SetNextWindowClass(parent->getWindowClass());
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextWindowSize(ImVec2{avail.x / 2, avail.y / 2}, ImGuiCond_Once);
  if (ImGui::Begin(idIfy(mName).c_str(), &bOpen, mFlags)) {
    ImGui::PushID(mName.c_str() /*mId*/);

    if (mbDrawDockspace) {
      const ImGuiID dockspaceId =
          ImGui::GetID(idIfyChild("###DockSpace").c_str());
      if (!ImGui::DockBuilderGetNode(dockspaceId)) {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);

        ImGuiID next = dockspaceId;
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(
            next, ImGuiDir_Right, 0.2f, nullptr, &next);
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(
            next, ImGuiDir_Left, 0.4f, nullptr, &next);
        ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(
            dock_right_id, ImGuiDir_Down, 0.2f, nullptr, &dock_right_id);

        ImGui::DockBuilderDockWindow(idIfyChild("Outliner").c_str(),
                                     dock_right_id);
        ImGui::DockBuilderDockWindow(idIfyChild("Texture Preview").c_str(),
                                     dock_right_down_id);
        ImGui::DockBuilderDockWindow(idIfyChild("History").c_str(),
                                     dock_right_down_id);
        ImGui::DockBuilderDockWindow(idIfyChild("Property Editor").c_str(),
                                     dock_left_id);
        ImGui::DockBuilderDockWindow(idIfyChild("Viewport").c_str(), next);

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
