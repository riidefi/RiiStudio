#pragma once

#include "StudioWindow.hpp"           // StudioWindow
#include <core/3d/Texture.hpp>        // lib3d::Texture
#include <core/3d/ui/IconManager.hpp> // IconManager
#include <core/kpi/Node.hpp>          // kpi::IDocumentNode
#include <unordered_map>              // std::unordered_map

namespace riistudio::frontend {

class EditorWindow : public StudioWindow {
public:
  EditorWindow(std::unique_ptr<kpi::IDocumentNode> state,
               const std::string& path);

  ImGuiID buildDock(ImGuiID root_id) override;

  void draw_() override;

  std::string getFilePath() { return mFilePath; }

  std::unique_ptr<kpi::IDocumentNode> mState;
  std::string mFilePath;
  kpi::History mHistory;
  kpi::IDocumentNode* mActive = nullptr;

  void propogateIcons(kpi::FolderData& folder) {
    for (int i = 0; i < folder.size(); ++i) {
      auto& elem = folder[i];

      if (lib3d::Texture* tex = dynamic_cast<lib3d::Texture*>(elem.get());
          tex != nullptr) {
        mImageIcons.emplace(tex, mIconManager.addIcon(*tex));
      }

      propogateIcons(*elem.get());
    }
  }
  void propogateIcons(kpi::IDocumentNode& node) {
    for (auto& [key, f] : node.children)
      propogateIcons(f);
  }

  void drawImageIcon(lib3d::Texture* tex, u32 dim) {
    if (auto icon = mImageIcons.find(tex); icon != mImageIcons.end())
      mIconManager.drawIcon(icon->second, dim, dim);
  }

private:
  IconManager mIconManager;
  std::unordered_map<lib3d::Texture*, IconManager::Key> mImageIcons;
};

} // namespace riistudio::frontend
