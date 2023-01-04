#include "EditorDocument.hpp"
#include <core/util/oishii.hpp> // OishiiFlushWriter
#include <plate/Platform.hpp>   // plate::Platform
#include <plugins/api.hpp>      // SpawnExporter

namespace riistudio::frontend {

EditorDocument::EditorDocument(std::unique_ptr<kpi::INode> state,
                               const std::string_view path)
    : kpi::Document<kpi::INode>(std::move(state)), mFilePath(path) {}
EditorDocument::~EditorDocument() {}

void EditorDocument::save() { saveAs(mFilePath); }
void EditorDocument::saveAs(const std::string_view _path) {
  std::string path(_path);
  if (path.ends_with(".bdl")) {
    path.resize(path.size() - 4);
    path += ".bmd";
  }
  oishii::Writer writer(0);

  auto ex = SpawnExporter(getRoot());
  if (!ex) {
    DebugReport("Failed to spawn exporter.\n");
    return;
  }
  ex->write_(getRoot(), writer);

  OishiiFlushWriter(writer, path);
}

} // namespace riistudio::frontend
