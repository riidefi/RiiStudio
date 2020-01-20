#pragma once

#include <LibRiiEditor/pluginapi/Plugin.hpp>
#include <LibRiiEditor/ui/widgets/Outliner.hpp>
#include <LibRiiEditor/core/WindowManager.hpp>

#include <map>
#include <vector>

#include <LibRiiEditor/core/PluginFactory.hpp>

struct IWindowsCollection
{
	virtual ~IWindowsCollection() = default;

	virtual u32 getNum() = 0;
	virtual const char* getName(u32 id) = 0;
	virtual std::unique_ptr<Window> spawn(u32 id) = 0;
};

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(std::unique_ptr<pl::FileState> state, PluginFactory& factory, const std::string& path);

	void draw(WindowContext* ctx) noexcept override final;

	std::unique_ptr<pl::FileState> mState;

	std::string mFilePath; // For saving

	//! What windows can we spawn?
	//! This won't be enough when we support multi interface editors.
	//!
	std::unique_ptr<IWindowsCollection> mWindowCollection;
};
