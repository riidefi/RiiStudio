#pragma once

#include <memory>
#include <LibCore/common.h>
#include <RiiStudio/IStudioWindow.hpp>

#include <LibCore/api/Node.hpp>

struct IWindowsCollection
{
	virtual ~IWindowsCollection() = default;

	virtual u32 getNum() = 0;
	virtual const char* getName(u32 id) = 0;
	virtual std::unique_ptr<IStudioWindow> spawn(u32 id) = 0;
};

class EditorWindow : public IStudioWindow
{
public:
	EditorWindow(px::Dynamic state, const std::string& path, Window* parent);

	void draw() noexcept override;

	std::string getFilePath() { return mFilePath; }
	px::Dynamic& getState() { return mState; }

	bool showCursor = true;

private:
	px::Dynamic mState;
	std::string mFilePath;
};
