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

class EditorWindow : public StudioWindow,
                     public EditorDocument,
                     public IEditor {
public:
  EditorWindow(std::unique_ptr<kpi::INode> state, const std::string& path);

  EditorWindow(const EditorWindow&) = delete;
  EditorWindow(EditorWindow&&) = delete;

public:
  ~EditorWindow();

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  void drawImageIcon(const lib3d::Texture* tex, u32 dim) {
    mIconManager.drawImageIcon(tex, dim);
  }

  std::string getFilePath() const { return std::string(getPath()); }
  EditorDocument& getDocument() { return *this; }
  const EditorDocument& getDocument() const { return *this; }

  void reinit() {
    detachAllChildren();
    init();
  }

  SelectionManager& getSelection() { return mSelection; }
  void saveButton() override {
    rsl::trace("Attempting to save to {}", getFilePath());
    if (getFilePath().empty()) {
      saveAsButton();
      return;
    }
    saveAs(getFilePath());
  }
  void saveAsButton() override {
    std::vector<std::string> filters;
    const kpi::INode* node = &getDocument().getRoot();

    auto default_filename = std::filesystem::path(getFilePath()).filename();
    if (dynamic_cast<const riistudio::j3d::Collection*>(node) != nullptr) {
      filters.push_back("Binary Model Data (*.bmd)"_j);
      filters.push_back("*.bmd");
      default_filename.replace_extension(".bmd");
    } else if (dynamic_cast<const riistudio::g3d::Collection*>(node) !=
               nullptr) {
      filters.push_back("Binary Resource (*.brres)"_j);
      filters.push_back("*.brres");
      default_filename.replace_extension(".brres");
    }

    filters.push_back("All Files");
    filters.push_back("*");

    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results)
      return;
    auto path = results->string();

    if (dynamic_cast<const riistudio::j3d::Collection*>(node) != nullptr) {
      if (!path.ends_with(".bmd"))
        path.append(".bmd");
    } else if (dynamic_cast<const riistudio::g3d::Collection*>(node) !=
               nullptr) {
      if (!path.ends_with(".brres"))
        path.append(".brres");
    }
    rsl::trace("Saving to {}", path);
    saveAs(path);
  }

private:
  void init();

  IconManager mIconManager;
  SelectionManager mSelection;

  std::string discordStatus() const override {
    if (auto* g = dynamic_cast<const riistudio::g3d::Collection*>(
            &getDocument().getRoot())) {
      return "Editing a BRRES";
    }
    if (auto* g = dynamic_cast<const riistudio::j3d::Collection*>(
            &getDocument().getRoot())) {
      return "Editing a BMD";
    }
    return "Working on unknown things";
  }
  void saveAs(std::string_view path) { getDocument().saveAs(path); }
};

} // namespace riistudio::frontend
