#include "EditorWindow.hpp"
#include <core/3d/i3dmodel.hpp>                            // lib3d::Scene
#include <frontend/applet.hpp>                             // core::Applet
#include <frontend/legacy_editor/views/HistoryList.hpp>    // MakePropertyEditor
#include <frontend/legacy_editor/views/Outliner.hpp>       // MakeHistoryList
#include <frontend/legacy_editor/views/PropertyEditor.hpp> // MakeOutliner
#include <frontend/legacy_editor/views/ViewportRenderer.hpp> // MakeViewportRenderer
#include <imcxx/Widgets.hpp>
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_TIMES

#include <plugins/g3d/G3dIo.hpp>
#include <plugins/j3d/J3dIo.hpp>

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
    mIconManager.drawImageIcon(tex, dim);
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
  attachWindow(MakeViewportRenderer(mDocument.getRoot()));
}

// We handle the Dockspace manually (disabling it in an error state)
BRRESEditor::BRRESEditor(std::string path)
    : StudioWindow(getFileShort(path), DockSetting::None),
      mDocument(nullptr), mPath(path) {}
BRRESEditor::BRRESEditor(std::span<const u8> span, const std::string& path)
    : StudioWindow(getFileShort(path), DockSetting::None),
      mDocument(nullptr), mPath(path) {
  auto out = std::make_unique<g3d::Collection>();
  oishii::BinaryReader reader(span, path, std::endian::big);
  kpi::LightIOTransaction trans;
  std::string total;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    total += msg + "\n";
    rsl::error(msg);
    Message emsg(message_class, std::string(domain), std::string(message_body));
    mLoadErrorMessages.push_back(emsg);
  };
  g3d::ReadBRRES(*out, reader, trans);
  if (trans.state != kpi::TransactionState::Complete) {
    mErrorState = true;
    return;
  }
  attach(std::move(out));
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
  if (mErrorState) {
    mLoadErrors.Draw(mLoadErrorMessages);
    return;
  }
  drawDockspace();
  if (!mLoadErrorMessages.empty()) {
    auto name = idIfyChild("Warnings");
    bool open = true;
    if (ImGui::Begin(name.c_str(), &open)) {
      mLoadErrors.Draw(mLoadErrorMessages);
      if (ImGui::Button("OK")) {
        open = false;
      }
    }
    ImGui::End();
    if (!open) {
      mLoadErrorMessages.clear();
    }
  }
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
    mIconManager.drawImageIcon(tex, dim);
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
  attachWindow(MakeViewportRenderer(mDocument.getRoot()));
}
void BRRESEditor::saveAsImpl(std::string path) {
  oishii::Writer writer(std::endian::big);
  auto& col = mDocument.getRoot();
  if (auto ok = g3d::WriteBRRES(col, writer); !ok) {
    rsl::ErrorDialogFmt("Failed to rebuild BRRES: {}", ok.error());
    return;
  }
  writer.saveToDisk(path);
}

// We handle the Dockspace manually (disabling it in an error state)
BMDEditor::BMDEditor(std::string path)
    : StudioWindow(getFileShort(path), DockSetting::None),
      mDocument(nullptr), mPath(path) {}
BMDEditor::BMDEditor(std::span<const u8> span, const std::string& path)
    : StudioWindow(getFileShort(path), DockSetting::None),
      mDocument(nullptr), mPath(path) {
  auto out = std::make_unique<j3d::Collection>();
  oishii::BinaryReader reader(span, path, std::endian::big);
  kpi::LightIOTransaction trans;
  std::string total;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    total += msg + "\n";
    rsl::error(msg);
    Message emsg(message_class, std::string(domain), std::string(message_body));
    mLoadErrorMessages.push_back(emsg);
  };
  auto ok = j3d::ReadBMD(*out, reader, trans);
  if (!ok) {
    rsl::error(ok.error());
    mErrorState = true;
    return;
  }
  if (trans.state != kpi::TransactionState::Complete) {
    rsl::ErrorDialog(total);
    return;
  }
  attach(std::move(out));
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
  if (mErrorState) {
    mLoadErrors.Draw(mLoadErrorMessages);
    return;
  }
  drawDockspace();
  if (!mLoadErrorMessages.empty()) {
    auto name = idIfyChild("Warnings");
    bool open = true;
    if (ImGui::Begin(name.c_str(), &open)) {
      mLoadErrors.Draw(mLoadErrorMessages);
      if (ImGui::Button("OK")) {
        open = false;
      }
    }
    ImGui::End();
    if (!open) {
      mLoadErrorMessages.clear();
    }
  }
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
void BMDEditor::saveAsImpl(std::string path) {
  if (path.ends_with(".bdl")) {
    path.resize(path.size() - 4);
    path += ".bmd";
  }
  oishii::Writer writer(std::endian::big);
  auto& col = mDocument.getRoot();
  if (auto ok = j3d::WriteBMD(col, writer); !ok) {
    rsl::ErrorDialogFmt("Failed to rebuild BMD: {}", ok.error());
    return;
  }
  writer.saveToDisk(path);
}

} // namespace riistudio::frontend
