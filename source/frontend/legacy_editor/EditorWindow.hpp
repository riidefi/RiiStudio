#pragma once

#include <rsl/FsDialog.hpp>

#include "StudioWindow.hpp"               // StudioWindow
#include <LibBadUIFramework/Document.hpp> // kpi::Document
#include <LibBadUIFramework/Node2.hpp>    // kpi::INode
#include <LibBadUIFramework/Plugins.hpp>  // kpi::IOTransaction
#include <core/3d/Texture.hpp>            // lib3d::Texture
#include <frontend/IEditor.hpp>
#include <frontend/widgets/ErrorDialogList.hpp>
#include <frontend/widgets/IconManager.hpp> // IconManager
#include <string_view>                      // std::string_view

#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/Scene.hpp>

namespace riistudio::frontend {

using SelectionManager = kpi::SelectionManager;

struct BRRESEditor : public StudioWindow, public IEditor {
  BRRESEditor(std::string path);
  BRRESEditor(std::span<const u8> buf, const std::string& path);
  BRRESEditor(const BRRESEditor&) = delete;
  BRRESEditor(BRRESEditor&&) = delete;
  ~BRRESEditor();

  void attach(auto&& out) {
    mDocument.setRoot(std::move(out));
    init();
  }

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  std::string getFilePath() const { return mPath; }

  void reinit() {
    detachAllChildren();
    init();
  }

  void init();

  kpi::Document<g3d::Collection> mDocument;
  std::string mPath;
  IconManager mIconManager;
  SelectionManager mSelection;
  ErrorDialogList mLoadErrors;
  std::vector<Message> mLoadErrorMessages;
  bool mErrorState = false;
  std::unique_ptr<StudioWindow> mPropertyEditor;
  std::unique_ptr<StudioWindow> mHistoryList;
  std::unique_ptr<StudioWindow> mOutliner;
  std::unique_ptr<StudioWindow> mRenderTest;

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
    saveAsImpl(path);
  }
  void saveButton() override {
    rsl::trace("Attempting to save to {}", getFilePath());
    if (getFilePath().empty()) {
      saveAsButton();
      return;
    }
    saveAsImpl(getFilePath());
  }
  void saveAsImpl(std::string path);
};

struct BMDEditor : public StudioWindow, public IEditor {
  BMDEditor(std::string path);
  BMDEditor(std::span<const u8> buf, const std::string& path);
  BMDEditor(const BMDEditor&) = delete;
  BMDEditor(BMDEditor&&) = delete;
  ~BMDEditor();

  void attach(auto&& out) {
    mDocument.setRoot(std::move(out));
    init();
  }

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  std::string getFilePath() const { return mPath; }

  void reinit() {
    detachAllChildren();
    init();
  }

  void init();

  kpi::Document<j3d::Collection> mDocument;
  std::string mPath;
  IconManager mIconManager;
  SelectionManager mSelection;
  ErrorDialogList mLoadErrors;
  std::vector<Message> mLoadErrorMessages;
  bool mErrorState = false;
  std::unique_ptr<StudioWindow> mPropertyEditor;
  std::unique_ptr<StudioWindow> mHistoryList;
  std::unique_ptr<StudioWindow> mOutliner;
  std::unique_ptr<StudioWindow> mRenderTest;

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
    saveAsImpl(path);
  }
  void saveButton() override {
    rsl::trace("Attempting to save to {}", getFilePath());
    if (getFilePath().empty()) {
      saveAsButton();
      return;
    }
    saveAsImpl(getFilePath());
  }
  void saveAsImpl(std::string path);
};

} // namespace riistudio::frontend
