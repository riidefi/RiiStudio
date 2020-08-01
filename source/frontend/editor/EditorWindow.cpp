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

  if (mShowMessages && !mMessages.empty()) {
    ImGui::OpenPopup("Log");
  }
  ImGui::SetNextWindowSize({800.0f, 0.0f}, ImGuiCond_Once);
  if (ImGui::BeginPopupModal("Log", &mShowMessages)) {
    ImGui::SetWindowFontScale(1.2f);
    const auto entry_flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
    if (ImGui::BeginTable("Body", 3, entry_flags)) {
      ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("Message Type");
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("Message Path");
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("Message Body");
      for (auto& msg : mMessages) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted([](kpi::IOMessageClass mclass) -> const char* {
          switch (mclass) {
          case kpi::IOMessageClass::None:
            return "Invalid";
          case kpi::IOMessageClass::Information:
            return (const char*)ICON_FA_BELL u8" Info";
          case kpi::IOMessageClass::Warning:
            return (const char*)ICON_FA_EXCLAMATION_TRIANGLE u8" Warning";
          case kpi::IOMessageClass::Error:
            return (const char*)ICON_FA_TIMES u8" Error";
          }
        }(msg.message_class));
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(msg.domain.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::TextWrapped("%s", msg.message_body.c_str());
      }

      ImGui::EndTable();
    }

    if (ImGui::Button("Close")) {
      ImGui::CloseCurrentPopup();
      mShowMessages = false;
    }
    ImGui::EndPopup();
  }
}

} // namespace riistudio::frontend
