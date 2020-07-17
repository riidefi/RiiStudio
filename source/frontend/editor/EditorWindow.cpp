#include "EditorWindow.hpp"
#include <core/3d/i3dmodel.hpp>                  // lib3d::Scene
#include <core/api.hpp>                          // SpawnExporter
#include <core/util/gui.hpp>                     // ImGui::DockBuilderDockWindow
#include <frontend/applet.hpp>                   // core::Applet
#include <frontend/editor/views/HistoryList.hpp> // MakePropertyEditor
#include <frontend/editor/views/Outliner.hpp>    // MakeHistoryList
#include <frontend/editor/views/PropertyEditor.hpp>   // MakeOutliner
#include <frontend/editor/views/ViewportRenderer.hpp> // MakeViewportRenderer
#include <fstream>                                    // ofstream
#include <oishii/reader/binary_reader.hxx>            // oishii::BinaryWriter
#include <oishii/writer/binary_writer.hxx>            // oishii::Writer
#include <string>                                     // std::string
#include <vendor/fa5/IconsFontAwesome5.h>             // ICON_FA_TIMES

namespace riistudio::frontend {

std::string getFileShort(const std::string& path) {
  return path.substr(path.rfind("\\") + 1);
}

void EditorWindow::init() {
  mDocument.commit();
  propogateIcons(mDocument.getRoot());

  attachWindow(MakePropertyEditor(mDocument.getHistory(), mDocument.getRoot(),
                                  mActive, *this));
  attachWindow(MakeHistoryList(mDocument.getHistory(), mDocument.getRoot()));
  attachWindow(MakeOutliner(mDocument.getRoot(), mActive, *this));
  if (dynamic_cast<lib3d::Scene*>(&mDocument.getRoot()) != nullptr)
    attachWindow(MakeViewportRenderer(mDocument.getRoot()));
}

EditorWindow::EditorWindow(std::unique_ptr<kpi::INode> state,
                           const std::string& path)
    : StudioWindow(getFileShort(path), true), mDocument(std::move(state)),
      mFilePath(path) {
  init();
}
EditorWindow::EditorWindow(FileData&& data)
    : StudioWindow(getFileShort(data.mPath), true), mFilePath(data.mPath) {
  // TODO: Not ideal..
  std::vector<u8> vec(data.mLen);
  memcpy(vec.data(), data.mData.get(), data.mLen);

  oishii::DataProvider provider(std::move(vec), data.mPath);
  oishii::BinaryReader reader(provider.slice());
  auto importer = SpawnImporter(data.mPath, provider.slice());

  if (!importer.second) {
    printf("Cannot spawn importer..\n");
    return;
  }
  if (!IsConstructible(importer.first)) {
    printf("Non constructable state.. find parents\n");

    const auto children = GetChildrenOfType(importer.first);
    if (children.empty()) {
      printf("No children. Cannot construct.\n");
      return;
    }
    assert(/*children.size() == 1 &&*/ IsConstructible(children[0])); // TODO
    importer.first = children[0];
  }

  std::unique_ptr<kpi::INode> fileState{
      dynamic_cast<kpi::INode*>(SpawnState(importer.first).release())};
  if (!fileState.get()) {
    printf("Cannot spawn file state %s.\n", importer.first.c_str());
    return;
  }
  getDocument().setRoot(std::move(fileState));
#if 0
  struct Handler : oishii::ErrorHandler {
    void onErrorBegin(const oishii::DataProvider& stream) override {
      puts("[Begin Error]");
    }
    void onErrorDescribe(const oishii::DataProvider& stream, const char* type,
                         const char* brief, const char* details) override {
      printf("- [Describe] Type %s, Brief: %s, Details: %s\n", type, brief,
             details);
    }
    void onErrorAddStackTrace(const oishii::DataProvider& stream,
                              std::streampos start, std::streamsize size,
                              const char* domain) override {
      printf("- [Stack] Start: %u, Size: %u, Domain: %s\n", (u32)start,
             (u32)size, domain);
    }
    void onErrorEnd(const oishii::DataProvider& stream) override {
      puts("[End Error]");
    }
  } handler;
  reader.addErrorHandler(&handler);
#endif
  auto message_handler = [&](kpi::IOMessageClass message_class,
                             const std::string_view domain,
                             const std::string_view message_body) {
    mMessages.emplace_back(message_class, std::string(domain),
                           std::string(message_body));
  };
  kpi::IOTransaction transaction{getDocument().getRoot(), provider.slice(),
                                 message_handler};
  importer.second->read_(transaction);

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

  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mDocument.undo();
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mDocument.redo();
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

void EditorWindow::save(const std::string_view path) {
  oishii::Writer writer(1024);

  auto ex = SpawnExporter(getDocument().getRoot());
  if (!ex) {
    DebugReport("Failed to spawn importer.\n");
    return;
  }
  ex->write_(getDocument().getRoot(), writer);

  plate::Platform::writeFile({writer.getDataBlockStart(), writer.getBufSize()},
                             path);
}

} // namespace riistudio::frontend
