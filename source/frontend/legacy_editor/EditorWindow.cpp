#include "EditorWindow.hpp"
#include <core/3d/i3dmodel.hpp>                            // lib3d::Scene
#include <frontend/applet.hpp>                             // core::Applet
#include <frontend/legacy_editor/views/HistoryList.hpp>    // MakePropertyEditor
#include <frontend/legacy_editor/views/Outliner.hpp>       // MakeHistoryList
#include <frontend/legacy_editor/views/PropertyEditor.hpp> // MakeOutliner
#include <frontend/legacy_editor/views/ViewportRenderer.hpp> // MakeViewportRenderer
#include <imcxx/Widgets.hpp>
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_TIMES

namespace riistudio::frontend {

static std::string getFileShort(const std::string& path) {
  const auto path_ = path.substr(path.rfind("\\") + 1);

  return path_.substr(path_.rfind("/") + 1);
}

void EditorWindow::init() {
  // Don't require selection reset on first element
  commit(mSelection, false);
  mIconManager.propagateIcons(getRoot());

  // mActive must be stable
  attachWindow(MakePropertyEditor(getHistory(), getRoot(), mSelection, *this));
  attachWindow(MakeHistoryList(getHistory(), getRoot(), mSelection));
  attachWindow(MakeOutliner(getRoot(), mSelection, *this));
  if (auto* scene = dynamic_cast<lib3d::Scene*>(&getRoot()))
    attachWindow(MakeViewportRenderer(*scene));
}

EditorWindow::EditorWindow(std::unique_ptr<kpi::INode> state,
                           const std::string& path)
    : StudioWindow(getFileShort(path), DockSetting::Dockspace),
      EditorDocument(std::move(state), path) {
  init();
}
EditorWindow::~EditorWindow() = default;
ImGuiID EditorWindow::buildDock(ImGuiID root_id) {
  ImGuiID next = root_id;
  ImGuiID dock_right_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 0.3f, nullptr, &next);
  ImGuiID dock_left_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.3f, nullptr, &next);
  ImGuiID dock_left_down_id = ImGui::DockBuilderSplitNode(
      dock_left_id, ImGuiDir_Down, 0.2f, nullptr, &dock_left_id);

  ImGui::DockBuilderDockWindow(idIfyChild("Outliner").c_str(), dock_left_id);
  ImGui::DockBuilderDockWindow(idIfyChild("History").c_str(),
                               dock_left_down_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Property Editor").c_str(),
                               dock_right_id);
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
      undo(mSelection);
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      redo(mSelection);
    }
  }
}

} // namespace riistudio::frontend
