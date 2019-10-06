#pragma once

#include <LibRiiEditor/ui/ThemeManager.hpp>
#include <LibRiiEditor/ui/Window.hpp>
#include "CoreContext.hpp"

class Theme
{
public:
	struct Editor
	{
		Editor() = default;
		~Editor() = default;

		const char* mTitle = "Theme Editor";

		void windowDraw(CoreContext& ctx);
	};

	ThemeManager::BasicTheme mThemeSelection = ThemeManager::BasicTheme::ImDark;
	ThemeManager mThemeManager;
};
