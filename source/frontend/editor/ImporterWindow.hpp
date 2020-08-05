#pragma once

#include <frontend/editor/EditorImporter.hpp> // EditorImporter

namespace riistudio::frontend {

class ImporterWindow : public EditorImporter {
public:
  ImporterWindow(FileData&& data) : EditorImporter(std::move(data)) {}
  ~ImporterWindow() = default;

  void draw();
  void drawMessages();

  bool attachEditor() const { return mDone && succeeded(); }
  bool abort() const { return mDone && !succeeded(); }

  bool acceptDrop() const { return result == State::ResolveDependencies; }
  void drop(FileData&& data);

private:
  bool mDone = false;
};

} // namespace riistudio::frontend
