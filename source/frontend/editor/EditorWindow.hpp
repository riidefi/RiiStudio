#pragma once

#include "EditorDocument.hpp"               // EditorDocument
#include "StudioWindow.hpp"                 // StudioWindow
#include <core/3d/Texture.hpp>              // lib3d::Texture
#include <LibBadUIFramework/Document.hpp>            // kpi::Document
#include <LibBadUIFramework/Node2.hpp>               // kpi::INode
#include <LibBadUIFramework/Plugins.hpp>             // kpi::IOTransaction
#include <frontend/widgets/IconManager.hpp> // IconManager
#include <string_view>                      // std::string_view

namespace riistudio::frontend {

using SelectionManager = kpi::SelectionManager;

class EditorWindow : public StudioWindow, public EditorDocument {
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
};

} // namespace riistudio::frontend
