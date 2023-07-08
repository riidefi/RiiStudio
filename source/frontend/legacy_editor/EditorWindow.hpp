#pragma once

#include <rsl/FsDialog.hpp>

#include "EditorDocument.hpp"             // EditorDocument
#include "StudioWindow.hpp"               // StudioWindow
#include <LibBadUIFramework/Document.hpp> // kpi::Document
#include <LibBadUIFramework/Node2.hpp>    // kpi::INode
#include <LibBadUIFramework/Plugins.hpp>  // kpi::IOTransaction
#include <core/3d/Texture.hpp>            // lib3d::Texture
#include <frontend/IEditor.hpp>
#include <frontend/widgets/IconManager.hpp> // IconManager
#include <string_view>                      // std::string_view

#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/Scene.hpp>

namespace riistudio::frontend {

using SelectionManager = kpi::SelectionManager;

struct BRRESEditor : public StudioWindow, public IEditor {
  BRRESEditor(std::unique_ptr<kpi::INode> state, const std::string& path);
  BRRESEditor(const BRRESEditor&) = delete;
  BRRESEditor(BRRESEditor&&) = delete;
  ~BRRESEditor();

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  void drawImageIcon(const lib3d::Texture* tex, u32 dim) {
    mIconManager.drawImageIcon(tex, dim);
  }

  std::string getFilePath() const { return std::string(mDocument.getPath()); }

  void reinit() {
    detachAllChildren();
    init();
  }

  void init();
  EditorDocument mDocument;
  IconManager mIconManager;
  SelectionManager mSelection;

  std::string discordStatus() const override { return "Editing a BRRES"; }
  void saveAsButton() override {
    std::vector<std::string> filters;
    auto default_filename = std::filesystem::path(getFilePath()).filename();

    filters.push_back("Binary Resource (*.brres)"_j);
    filters.push_back("*.brres");
    default_filename.replace_extension(".brres");

    filters.push_back("All Files");
    filters.push_back("*");

    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results)
      return;
    auto path = results->string();

    if (!path.ends_with(".brres"))
      path.append(".brres");
    rsl::trace("Saving to {}", path);
    mDocument.saveAs(path);
  }
  void saveButton() override {
    rsl::trace("Attempting to save to {}", getFilePath());
    if (getFilePath().empty()) {
      saveAsButton();
      return;
    }
    mDocument.saveAs(getFilePath());
  }
};

struct BMDEditor : public StudioWindow, public IEditor {
  BMDEditor(std::unique_ptr<kpi::INode> state, const std::string& path);
  BMDEditor(const BMDEditor&) = delete;
  BMDEditor(BMDEditor&&) = delete;
  ~BMDEditor();

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  void drawImageIcon(const lib3d::Texture* tex, u32 dim) {
    mIconManager.drawImageIcon(tex, dim);
  }

  std::string getFilePath() const { return std::string(mDocument.getPath()); }

  void reinit() {
    detachAllChildren();
    init();
  }

  void init();
  EditorDocument mDocument;
  IconManager mIconManager;
  SelectionManager mSelection;

  std::string discordStatus() const override { return "Editing a BMD"; }
  void saveAsButton() override {
    std::vector<std::string> filters;
    auto default_filename = std::filesystem::path(getFilePath()).filename();
    filters.push_back("Binary Model Data (*.bmd)"_j);
    filters.push_back("*.bmd");
    default_filename.replace_extension(".bmd");

    filters.push_back("All Files");
    filters.push_back("*");

    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results)
      return;
    auto path = results->string();

    if (!path.ends_with(".bmd"))
      path.append(".bmd");
    rsl::trace("Saving to {}", path);
    mDocument.saveAs(path);
  }
  void saveButton() override {
    rsl::trace("Attempting to save to {}", getFilePath());
    if (getFilePath().empty()) {
      saveAsButton();
      return;
    }
    mDocument.saveAs(getFilePath());
  }
};

} // namespace riistudio::frontend
