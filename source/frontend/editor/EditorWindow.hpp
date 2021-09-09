#pragma once

#include "EditorDocument.hpp"               // EditorDocument
#include "StudioWindow.hpp"                 // StudioWindow
#include <core/3d/Texture.hpp>              // lib3d::Texture
#include <core/kpi/Document.hpp>            // kpi::Document
#include <core/kpi/Node2.hpp>               // kpi::INode
#include <core/kpi/Plugins.hpp>             // kpi::IOTransaction
#include <frontend/widgets/IconManager.hpp> // IconManager
#include <llvm/ADT/SmallVector.h>           // llvm::SmallVector
#include <string_view>                      // std::string_view

namespace riistudio::frontend {

struct SelectionManager {
  kpi::IObject* getActive() { return mActive; }
  const kpi::IObject* getActive() const { return mActive; }
  void setActive(kpi::IObject* active) { mActive = active; }

  void select(kpi::IObject* obj) { selected.insert(obj); }
  void deselect(kpi::IObject* obj) { selected.erase(obj); }

  bool isSelected(kpi::IObject* obj) const { return selected.contains(obj); }

  std::set<kpi::IObject*> selected;
  kpi::IObject* mActive = nullptr;
};

class EditorWindow : public StudioWindow, public EditorDocument {
public:
  EditorWindow(std::unique_ptr<kpi::INode> state, const std::string& path);

  EditorWindow(const EditorWindow&) = delete;
  EditorWindow(EditorWindow&&) = delete;

public:
  ~EditorWindow() = default;

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  void drawImageIcon(const lib3d::Texture* tex, u32 dim) const {
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
