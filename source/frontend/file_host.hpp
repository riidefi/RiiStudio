#pragma once

#include <string>
#include <queue>
#include <memory>

#include <stdint.h>

#include <core/common.h>

namespace riistudio::frontend {

struct FileData
{
    std::unique_ptr<uint8_t[]> mData;
    std::size_t mLen;
    std::string mPath;

    FileData(std::unique_ptr<uint8_t[]> data, std::size_t len, const std::string& path)
        : mData(std::move(data)), mLen(len), mPath(path)
    {}
};

enum class OpenFilePolicy
{
    NewEditor,
    ReplaceEditorIfMatching,
    ReplaceEditor
};

class FileHost {
public:
    virtual ~FileHost() = default;
    virtual void onFileOpen(FileData data, OpenFilePolicy policy) = 0;
    
    // Called once per frame
    void fileHostProcess();

    // Call from UI
    void openFile(OpenFilePolicy policy = OpenFilePolicy::NewEditor);
	void openFile(const std::string& path, OpenFilePolicy policy = OpenFilePolicy::NewEditor);
	
    // Call from dropper
    void drop(const std::vector<std::string>& paths);
    void dropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len, const std::string& name);

    // Drag and drop..
    std::queue<std::string> mDropQueue;
	std::queue<FileData> mDataDropQueue;
};

}