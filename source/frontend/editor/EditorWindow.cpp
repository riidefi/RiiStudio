#include "EditorWindow.hpp"
#include <core/util/gui.hpp>                     // ImGui::DockBuilderDockWindow
#include <frontend/applet.hpp>                   // core::Applet
#include <frontend/editor/views/HistoryList.hpp> // MakePropertyEditor
#include <frontend/editor/views/Outliner.hpp>    // MakeHistoryList
#include <frontend/editor/views/PropertyEditor.hpp>   // MakeOutliner
#include <frontend/editor/views/ViewportRenderer.hpp> // MakeViewportRenderer

namespace riistudio::frontend {

EditorWindow::EditorWindow(std::unique_ptr<kpi::INode> state,
                           const std::string& path)
    : StudioWindow(path.substr(path.rfind("\\") + 1), true),
      mState(std::move(state)), mFilePath(path) {
  mHistory.commit(*mState.get());

  propogateIcons(*mState.get());

  attachWindow(MakePropertyEditor(mHistory, *mState.get(), mActive, *this));
  attachWindow(MakeHistoryList(mHistory, *mState.get()));
  attachWindow(MakeOutliner(*mState.get(), mActive, *this));
  attachWindow(MakeViewportRenderer(*mState.get()));
}
ImGuiID EditorWindow::buildDock(ImGuiID root_id) {
  ImGuiID next = root_id;
  ImGuiID dock_right_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 0.2f, nullptr, &next);
  ImGuiID dock_left_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.4f, nullptr, &next);
  ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(
      dock_right_id, ImGuiDir_Down, 0.2f, nullptr, &dock_right_id);

  ImGui::DockBuilderDockWindow(idIfyChild("Outliner").c_str(), dock_right_id);
  ImGui::DockBuilderDockWindow(idIfyChild("History").c_str(),
                               dock_right_down_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Property Editor").c_str(),
                               dock_left_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Viewport").c_str(), next);
  return next;
}
void EditorWindow::draw_() {
  detachClosedChildren();

  // TODO: This doesn't work
  frontend::Applet* parent = dynamic_cast<frontend::Applet*>(getParent());
  if (parent != nullptr) {
    if (parent->getActive() == this) {
      ImGui::Text("<Active>");
    } else {
      return;
    }
  }

  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mHistory.undo(*mState.get());
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mHistory.redo(*mState.get());
    }
  }
}
} // namespace riistudio::frontend
