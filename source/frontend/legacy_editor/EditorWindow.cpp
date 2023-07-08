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

void BRRESEditor::init() {
  // Don't require selection reset on first element
  mDocument.commit(mSelection, false);
  mIconManager.propagateIcons(mDocument.getRoot());

  auto draw_image_icon = [&](const lib3d::Texture* tex, u32 dim) {
    drawImageIcon(tex, dim);
  };
  auto post = [&]() { mDocument.commit(mSelection, true); };
  auto commit_ = [&](bool b) { mDocument.commit(mSelection, b); };
  // mActive must be stable
  attachWindow(MakePropertyEditor(mDocument.getHistory(), mDocument.getRoot(),
                                  mSelection, draw_image_icon));
  attachWindow(
      MakeHistoryList(mDocument.getHistory(), mDocument.getRoot(), mSelection));
  attachWindow(MakeOutliner(mDocument.getRoot(), mSelection, draw_image_icon,
                            post, commit_));
  if (auto* scene = dynamic_cast<lib3d::Scene*>(&mDocument.getRoot()))
    attachWindow(MakeViewportRenderer(*scene));
}

BRRESEditor::BRRESEditor(std::unique_ptr<kpi::INode> state,
                         const std::string& path)
    : StudioWindow(getFileShort(path), DockSetting::Dockspace),
      mDocument(std::move(state), path) {
  init();
}
BRRESEditor::~BRRESEditor() = default;
ImGuiID BRRESEditor::buildDock(ImGuiID root_id) {
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
void BRRESEditor::draw_() {
  detachClosedChildren();

  // TODO: Only affect active window
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mDocument.undo(mSelection);
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mDocument.redo(mSelection);
    }
  }
}

void BMDEditor::init() {
  // Don't require selection reset on first element
  mDocument.commit(mSelection, false);
  mIconManager.propagateIcons(mDocument.getRoot());

  auto draw_image_icon = [&](const lib3d::Texture* tex, u32 dim) {
    drawImageIcon(tex, dim);
  };
  auto post = [&]() { mDocument.commit(mSelection, true); };
  auto commit_ = [&](bool b) { mDocument.commit(mSelection, b); };
  // mActive must be stable
  attachWindow(MakePropertyEditor(mDocument.getHistory(), mDocument.getRoot(),
                                  mSelection, draw_image_icon));
  attachWindow(
      MakeHistoryList(mDocument.getHistory(), mDocument.getRoot(), mSelection));
  attachWindow(MakeOutliner(mDocument.getRoot(), mSelection, draw_image_icon,
                            post, commit_));
  if (auto* scene = dynamic_cast<lib3d::Scene*>(&mDocument.getRoot()))
    attachWindow(MakeViewportRenderer(*scene));
}

BMDEditor::BMDEditor(std::unique_ptr<kpi::INode> state, const std::string& path)
    : StudioWindow(getFileShort(path), DockSetting::Dockspace),
      mDocument(std::move(state), path) {
  init();
}
BMDEditor::~BMDEditor() = default;
ImGuiID BMDEditor::buildDock(ImGuiID root_id) {
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
void BMDEditor::draw_() {
  detachClosedChildren();

  // TODO: Only affect active window
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mDocument.undo(mSelection);
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mDocument.redo(mSelection);
    }
  }
}

} // namespace riistudio::frontend
