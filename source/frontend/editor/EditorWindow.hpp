#pragma once

#include "EditorDocument.hpp"         // EditorDocument
#include "StudioWindow.hpp"           // StudioWindow
#include <core/3d/Texture.hpp>        // lib3d::Texture
#include <core/3d/ui/IconManager.hpp> // IconManager
#include <core/kpi/Document.hpp>      // kpi::Document
#include <core/kpi/Node2.hpp>         // kpi::INode
#include <core/kpi/Plugins.hpp>       // kpi::IOTransaction
#include <llvm/ADT/SmallVector.h>     // llvm::SmallVector
#include <string_view>                // std::string_view

namespace riistudio::frontend {

class EditorWindow : public StudioWindow, public EditorDocument {
public:
  EditorWindow(std::unique_ptr<kpi::INode> state, const std::string& path);
  EditorWindow(FileData&& data);
  ~EditorWindow() = default;

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  void drawImageIcon(const lib3d::Texture* tex, u32 dim) const {
    mIconManager.drawImageIcon(tex, dim);
  }

  std::string getFilePath() const { return std::string(getPath()); }
  EditorDocument& getDocument() { return *this; }
  const EditorDocument& getDocument() const { return *this; }

  kpi::IObject* getActive() { return mActive; }
  const kpi::IObject* getActive() const { return mActive; }
  void setActive(kpi::IObject* active) { mActive = active; }

private:
  void init();

  IconManager mIconManager;
  kpi::IObject* mActive = nullptr;
  // bool mShowMessages = true;
};

} // namespace riistudio::frontend
