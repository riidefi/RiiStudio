#pragma once

#include "EditorImporter.hpp"
#include <core/kpi/Document.hpp>         // kpi::Document, kpi::INode
#include <core/kpi/Plugins.hpp>          // kpi::IOMessageClass
#include <frontend/file_host.hpp>        // FileData
#include <memory>                        // std::unique_ptr
#include <string>                        // std::string
#include <string_view>                   // std::string_view

namespace riistudio::frontend {

class EditorDocument : public kpi::Document<kpi::INode> {
public:
  //! Open a file
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
};

} // namespace riistudio::frontend
