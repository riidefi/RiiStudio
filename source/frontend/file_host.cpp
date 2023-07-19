#include "file_host.hpp"

#include <pfd/portable-file-dialogs.h>

IMPORT_STD;
#include <fstream>

namespace riistudio::frontend {

static std::vector<std::string> fileDialogueOpen() {
  return pfd::open_file("Open File").result();
}

// Called once per frame
void FileHost::fileHostProcess(
    std::function<void(FileData data, OpenFilePolicy policy)> onFileOpen) {
  while (!mDropQueue.empty()) {
    std::string to_open = mDropQueue.front();
    rsl::info("Reading from disc: {}", to_open.c_str());
    mDropQueue.pop();
    openFile(to_open, OpenFilePolicy::NewEditor);
  }
  while (!mDataDropQueue.empty()) {
    assert(!mDataDropQueue.front().mPath.empty() &&
           mDataDropQueue.front().mPath[0]);
    assert(onFileOpen);
    onFileOpen(std::move(mDataDropQueue.front()), OpenFilePolicy::NewEditor);
    mDataDropQueue.pop();
  }
}

// Call from UI
void FileHost::openFile(OpenFilePolicy policy) {
  auto results = fileDialogueOpen();

  if (results.empty())
    return;

  assert(results.size() == 1);
  if (results.size() != 1)
    return;

  auto file = results[0];
  openFile(file, policy);
}
std::optional<FileData> ReadFileData(const std::string& path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);

  if (!stream)
    return std::nullopt;

  const auto size = stream.tellg();
  stream.seekg(0, std::ios::beg);

  std::unique_ptr<u8[]> data = std::make_unique<u8[]>(size);
  if (!stream.read(reinterpret_cast<char*>(data.get()), size))
    return std::nullopt;

  return FileData{std::move(data), static_cast<std::size_t>(size), path};
}
void FileHost::openFile(const std::string& path, OpenFilePolicy policy) {
  auto drop = ReadFileData(path);
  if (!drop)
    return;

  mDataDropQueue.emplace(std::move(*drop));
}

// Call from dropper
void FileHost::drop(const std::vector<std::string>& paths) {
  for (const auto& path : paths) {
    rsl::info("Dropping file: {}", path.c_str());
    mDropQueue.push(path);
  }
}
void FileHost::dropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len,
                          const std::string& name) {
  rsl::info("Dropping file.. {}", name.c_str());
  FileData drop{std::move(data), len, name};
  mDataDropQueue.emplace(std::move(drop));
}

} // namespace riistudio::frontend
