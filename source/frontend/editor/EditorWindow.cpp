#include "EditorWindow.hpp"
#include <core/3d/i3dmodel.hpp>                  // lib3d::Scene
#include <core/util/gui.hpp>                     // ImGui::DockBuilderDockWindow
#include <frontend/applet.hpp>                   // core::Applet
#include <frontend/editor/views/HistoryList.hpp> // MakePropertyEditor
#include <frontend/editor/views/Outliner.hpp>    // MakeHistoryList
#include <frontend/editor/views/PropertyEditor.hpp>   // MakeOutliner
#include <frontend/editor/views/ViewportRenderer.hpp> // MakeViewportRenderer
#include <vendor/fa5/IconsFontAwesome5.h>             // ICON_FA_TIMES

namespace riistudio::frontend {

static std::string getFileShort(const std::string& path) {
  return path.substr(path.rfind("\\") + 1);
}

void EditorWindow::init() {
  commit();
  mIconManager.propogateIcons(getRoot());

  attachWindow(MakePropertyEditor(getHistory(), getRoot(), mActive, *this));
  attachWindow(MakeHistoryList(getHistory(), getRoot()));
  attachWindow(MakeOutliner(getRoot(), mActive, *this));
  if (dynamic_cast<lib3d::Scene*>(&getRoot()) != nullptr)
    attachWindow(MakeViewportRenderer(getRoot()));
}

EditorWindow::EditorWindow(std::unique_ptr<kpi::INode> state,
                           const std::string& path)
    : StudioWindow(getFileShort(path), true),
      EditorDocument(std::move(state), path) {
  init();
}
EditorWindow::EditorWindow(FileData&& data)
    : StudioWindow(getFileShort(data.mPath), true),
      EditorDocument(std::move(data)) {
  init();
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

#if 0
  // TODO: This doesn't work
  frontend::Applet* parent = dynamic_cast<frontend::Applet*>(getParent());
  if (parent != nullptr) {
    if (parent->getActive() == this) {
      ImGui::Text("<Active>");
    } else {
      return;
    }
  }
#endif

  // TODO: Only affect active window
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      undo();
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      redo();
    }
  }

}

} // namespace riistudio::frontend
