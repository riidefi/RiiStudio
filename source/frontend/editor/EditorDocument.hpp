#pragma once

#include <core/kpi/Document.hpp>         // kpi::Document, kpi::INode
#include <core/kpi/Plugins.hpp>          // kpi::IOMessageClass
#include <frontend/file_host.hpp>        // FileData
#include <memory>                        // std::unique_ptr
#include <string>                        // std::string
#include <string_view>                   // std::string_view
#include <vendor/llvm/ADT/SmallVector.h> // llvm::SmallVector

namespace riistudio::frontend {

class EditorDocument : public kpi::Document<kpi::INode> {
public:
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

  std::string_view getPath() const { return mFilePath; }

private:
  std::string mFilePath;

protected:
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
};

} // namespace riistudio::frontend
