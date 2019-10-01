#pragma once

#include "LibRiiEditor/pluginapi/Plugin.hpp"
#include "LibRiiEditor/ui/widgets/Outliner.hpp"
#include "WindowManager.hpp"

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(const pl::FileStateSpawner& registration);

	void draw(WindowContext* ctx) noexcept override final;

	std::unique_ptr<pl::FileState> mState;
};
