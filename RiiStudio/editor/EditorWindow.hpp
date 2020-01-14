#pragma once

#include <LibRiiEditor/pluginapi/Plugin.hpp>
#include <LibRiiEditor/ui/widgets/Outliner.hpp>
#include <LibRiiEditor/core/WindowManager.hpp>

#include <map>
#include <vector>

#include <LibRiiEditor/core/PluginFactory.hpp>

//struct ISelectionListener {};

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(std::unique_ptr<pl::FileState> state, PluginFactory& factory, const std::string& path);

	void draw(WindowContext* ctx) noexcept override final;

	std::unique_ptr<pl::FileState> mState;

	std::string mFilePath; // For saving

	// std::map<int, std::vector<ISelectionListener*>> mAttachedSelectionListeners;
};
