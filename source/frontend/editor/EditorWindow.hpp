#pragma once

#include "StudioWindow.hpp"           // StudioWindow
#include <core/3d/Texture.hpp>        // lib3d::Texture
#include <core/3d/ui/IconManager.hpp> // IconManager
#include <core/kpi/Node.hpp>          // kpi::IDocumentNode
#include <core/kpi/Node2.hpp>         // kpi::INode
#include <unordered_map>              // std::unordered_map

namespace riistudio::frontend {

class EditorWindow : public StudioWindow {
public:
  EditorWindow(std::unique_ptr<kpi::INode> state, const std::string& path);

  ImGuiID buildDock(ImGuiID root_id) override;

  void draw_() override;

  std::string getFilePath() { return mFilePath; }

  std::unique_ptr<kpi::INode> mState;
  std::string mFilePath;
  kpi::History mHistory;
  kpi::IObject* mActive = nullptr;

  void propogateIcons(kpi::ICollection& folder) {
    for (int i = 0; i < folder.size(); ++i) {
      kpi::IObject* elem = folder.atObject(i);

      if (lib3d::Texture* tex = dynamic_cast<lib3d::Texture*>(elem);
          tex != nullptr) {
        mImageIcons.emplace(tex, mIconManager.addIcon(*tex));
      }

      if (kpi::INode* node = dynamic_cast<kpi::INode*>(elem); node != nullptr) {
        propogateIcons(*node);
      }
    }
  }
  void propogateIcons(kpi::INode& node) {
    for (int i = 0; i < node.numFolders(); ++i)
      propogateIcons(*node.folderAt(i));
  }

  void drawImageIcon(const lib3d::Texture* tex, u32 dim) {
    if (auto icon = mImageIcons.find(const_cast<lib3d::Texture*>(tex));
        icon != mImageIcons.end())
      mIconManager.drawIcon(icon->second, dim, dim);
  }

private:
  IconManager mIconManager;
  std::unordered_map<lib3d::Texture*, IconManager::Key> mImageIcons;
};

} // namespace riistudio::frontend
