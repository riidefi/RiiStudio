#pragma once

#include <LibRiiEditor/pluginapi/Plugin.hpp>
#include <LibRiiEditor/ui/widgets/Outliner.hpp>
#include <LibRiiEditor/core/WindowManager.hpp>

#include <map>
#include <vector>

//struct ISelectionListener {};

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(std::unique_ptr<pl::FileState> state);

	void draw(WindowContext* ctx) noexcept override final;

	std::unique_ptr<pl::FileState> mState;

	// std::map<int, std::vector<ISelectionListener*>> mAttachedSelectionListeners;
};
