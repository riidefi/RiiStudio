#pragma once

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

private:
  void init();

  IconManager mIconManager;
  SelectionManager mSelection;

  std::string discordStatus() const override {
    if (auto* g = dynamic_cast<const riistudio::g3d::Collection*>(&getDocument().getRoot())) {
      return "Editing a BRRES";
    }
    if (auto* g = dynamic_cast<const riistudio::j3d::Collection*>(&getDocument().getRoot())) {
      return "Editing a BMD";
    }
    return "Working on unknown things";
  }
  void openFile(std::span<const u8> buf, std::string_view path) override {
    rsl::error("Cannot open {}", path);
  }
  void saveAs(std::string_view path) override {
    getDocument().saveAs(path);
  }
};

} // namespace riistudio::frontend
