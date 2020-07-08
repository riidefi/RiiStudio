#pragma once

#include "StudioWindow.hpp"           // StudioWindow
#include <core/3d/Texture.hpp>        // lib3d::Texture
#include <core/3d/ui/IconManager.hpp> // IconManager
#include <core/kpi/Document.hpp>      // kpi::Document
#include <core/kpi/Node2.hpp>         // kpi::INode
#include <core/kpi/Plugins.hpp>       // kpi::IOTransaction
#include <frontend/file_host.hpp>     // FileData
#include <llvm/ADT/SmallVector.h>     // llvm::SmallVector
#include <string_view>                // std::string_view

namespace riistudio::frontend {

class EditorDocument : public kpi::Document {
  //! Open a file
  EditorDocument(FileData&& data);
  EditorDocument(std::unique_ptr<kpi::INode> state,
                 const std::string_view path);
  //! Close a file
  ~EditorDocument();

  //! Save to the original location.
  void save();
  //! Save to the specified location.
  void saveAs(const std::string_view path);

private:
  std::string mFilePath;
};

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

  struct Message {
    kpi::IOMessageClass message_class;
    std::string domain;
    std::string message_body;

    Message(kpi::IOMessageClass mclass, std::string&& mdomain,
            std::string&& body)
        : message_class(mclass), domain(std::move(mdomain)),
          message_body(std::move(body)) {}
  };
  llvm::SmallVector<Message, 16> mMessages;
  bool mShowMessages = true;
};

} // namespace riistudio::frontend
