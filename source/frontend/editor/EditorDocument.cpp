#include "EditorDocument.hpp"
#include <core/api.hpp>                    // SpawnExporter
#include <oishii/reader/binary_reader.hxx> // oishii::BinaryReader
#include <oishii/writer/binary_writer.hxx> // oishii::Writer
#include <plate/Platform.hpp>              // plate::Platform

namespace riistudio::frontend {

EditorDocument::EditorDocument(FileData&& data) {
  // TODO: Not ideal..
  std::vector<u8> vec(data.mLen);
  memcpy(vec.data(), data.mData.get(), data.mLen);

  oishii::DataProvider provider(std::move(vec), data.mPath);
  oishii::BinaryReader reader(provider.slice());
  auto importer = SpawnImporter(data.mPath, provider.slice());

  if (!importer.second) {
    printf("Cannot spawn importer..\n");
    return;
  }
  if (!IsConstructible(importer.first)) {
    printf("Non constructable state.. find parents\n");

    const auto children = GetChildrenOfType(importer.first);
    if (children.empty()) {
      printf("No children. Cannot construct.\n");
      return;
    }
    assert(/*children.size() == 1 &&*/ IsConstructible(children[0])); // TODO
    importer.first = children[0];
  }

  std::unique_ptr<kpi::INode> fileState{
      dynamic_cast<kpi::INode*>(SpawnState(importer.first).release())};
  if (!fileState.get()) {
    printf("Cannot spawn file state %s.\n", importer.first.c_str());
    return;
  }
  setRoot(std::move(fileState));
#if 0
  struct Handler : oishii::ErrorHandler {
    void onErrorBegin(const oishii::DataProvider& stream) override {
      puts("[Begin Error]");
    }
    void onErrorDescribe(const oishii::DataProvider& stream, const char* type,
                         const char* brief, const char* details) override {
      printf("- [Describe] Type %s, Brief: %s, Details: %s\n", type, brief,
             details);
    }
    void onErrorAddStackTrace(const oishii::DataProvider& stream,
                              std::streampos start, std::streamsize size,
                              const char* domain) override {
      printf("- [Stack] Start: %u, Size: %u, Domain: %s\n", (u32)start,
             (u32)size, domain);
    }
    void onErrorEnd(const oishii::DataProvider& stream) override {
      puts("[End Error]");
    }
  } handler;
  reader.addErrorHandler(&handler);
#endif
  auto message_handler = [&](kpi::IOMessageClass message_class,
                             const std::string_view domain,
                             const std::string_view message_body) {
    mMessages.emplace_back(message_class, std::string(domain),
                           std::string(message_body));
  };
  kpi::IOTransaction transaction{getRoot(), provider.slice(), message_handler};
  importer.second->read_(transaction);
}
EditorDocument::EditorDocument(std::unique_ptr<kpi::INode> state,
                               const std::string_view path)
    : kpi::Document<kpi::INode>(std::move(state)), mFilePath(path) {}
EditorDocument ::~EditorDocument() {}

void EditorDocument::save() { saveAs(mFilePath); }
void EditorDocument::saveAs(const std::string_view _path) {
  std::string path(_path);
  if (path.ends_with(".bdl")) {
    path.resize(path.size() - 4);
    path += ".bmd";
  }
  oishii::Writer writer(1024);

  auto ex = SpawnExporter(getRoot());
  if (!ex) {
    DebugReport("Failed to spawn exporter.\n");
    return;
  }
  ex->write_(getRoot(), writer);

  plate::Platform::writeFile({writer.getDataBlockStart(), writer.getBufSize()},
                             path);
}

} // namespace riistudio::frontend
