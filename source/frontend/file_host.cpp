#include "file_host.hpp"

#include <vendor/pfd/portable-file-dialogs.h>

#include <fstream>

namespace riistudio::frontend {

static std::vector<std::string> fileDialogueOpen()
{
	return pfd::open_file("Open File").result();	
}

// Called once per frame
void FileHost::fileHostProcess() {
    while (!mDropQueue.empty())
	{
		openFile(mDropQueue.back(), OpenFilePolicy::NewEditor);
		mDropQueue.pop();
	}
	while (!mDataDropQueue.empty())
	{
		printf("Opening file..\n");
		onFileOpen(std::move(mDataDropQueue.back()), OpenFilePolicy::NewEditor);
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
void FileHost::openFile(const std::string& path, OpenFilePolicy policy) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);

	if (!stream)
		return;

	const auto size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	std::unique_ptr<u8[]> data = std::make_unique<u8[]>(size);
	if (!stream.read(reinterpret_cast<char*>(data.get()), size))
        return;
        
	FileData drop{ std::move(data), static_cast<std::size_t>(size), path };
	mDataDropQueue.emplace(std::move(drop));
}

// Call from dropper
void FileHost::drop(const std::vector<std::string>& paths) {
    for (const auto& path : paths)
	{
		DebugReport("Dropping file: %s\n", path.data());
		mDropQueue.push(std::string(path));
	}
}
void FileHost::dropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len, const std::string& name) {
    printf("Dropping file.. %s\n", name.c_str());
	FileData drop{ std::move(data), len, name };
	mDataDropQueue.emplace(std::move(drop));
}

} // namespace riistudio::frontend
