#pragma once

#include <memory>
#include <optional>
#include <queue>
#include <stdint.h>
#include <string.h>
#include <string>

#include <core/common.h>

namespace riistudio::frontend {

struct FileData {
  std::unique_ptr<uint8_t[]> mData;
  std::size_t mLen;
  std::string mPath;

  FileData(std::unique_ptr<uint8_t[]> data, std::size_t len,
           const std::string& path)
      : mData(std::move(data)), mLen(len), mPath(path) {}
  FileData() = default;
};

std::optional<FileData> ReadFileData(const std::string& path);

enum class OpenFilePolicy {
  NewEditor,
  ReplaceEditorIfMatching,
  ReplaceEditor,
};

class FileHost {
public:
  // Called once per frame
  void fileHostProcess(
      std::function<void(FileData data, OpenFilePolicy policy)> onFileOpen);

  // Call from UI
  void openFile(OpenFilePolicy policy = OpenFilePolicy::NewEditor);
  void openFile(const std::string& path,
                OpenFilePolicy policy = OpenFilePolicy::NewEditor);

  // Call from dropper
  void drop(const std::vector<std::string>& paths);
  void dropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len,
                  const std::string& name);
  void dropDirect(std::vector<uint8_t> data, const std::string& name) {
    auto ptr = std::make_unique<uint8_t[]>(data.size());
    memcpy(ptr.get(), data.data(), data.size());
    dropDirect(std::move(ptr), data.size(), name);
  }

  // Drag and drop..
  std::queue<std::string> mDropQueue;
  std::queue<FileData> mDataDropQueue;
};

} // namespace riistudio::frontend
