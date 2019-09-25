#pragma once

#include "pluginapi/Plugin.hpp"
#include "ui/widgets/Outliner.hpp"

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(const pl::FileEditor& registration);

	void draw(WindowContext* ctx) noexcept override final;

	const pl::FileEditor& mEditor;
};