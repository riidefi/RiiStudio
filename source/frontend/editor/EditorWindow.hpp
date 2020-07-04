#pragma once

#include "StudioWindow.hpp"           // StudioWindow
#include <core/3d/Texture.hpp>        // lib3d::Texture
#include <core/3d/ui/IconManager.hpp> // IconManager
#include <core/kpi/Document.hpp>      // kpi::Document
#include <core/kpi/Node2.hpp>         // kpi::INode
#include <frontend/file_host.hpp>     // FileData
#include <string_view>                // std::string_view

namespace riistudio::frontend {

class EditorWindow : public StudioWindow {
public:
  EditorWindow(std::unique_ptr<kpi::INode> state, const std::string& path);
  EditorWindow(FileData&& data);
  ~EditorWindow() = default;

  ImGuiID buildDock(ImGuiID root_id) override;
  void draw_() override;
  void drawImageIcon(const lib3d::Texture* tex, u32 dim) const {
    mIconManager.drawImageIcon(tex, dim);
  }

  std::string getFilePath() { return mFilePath; }
  kpi::Document& getDocument() { return mDocument; }
  const kpi::Document& getDocument() const { return mDocument; }

  kpi::IObject* getActive() { return mActive; }
  const kpi::IObject* getActive() const { return mActive; }
  void setActive(kpi::IObject* active) { mActive = active; }

  void save(const std::string_view path);

private:
  void propogateIcons(kpi::INode& node) { mIconManager.propogateIcons(node); }
  void init();

private:
  kpi::Document mDocument;
  IconManager mIconManager;
  kpi::IObject* mActive = nullptr;
  std::string mFilePath;
};

} // namespace riistudio::frontend
