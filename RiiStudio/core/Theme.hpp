#pragma once

#include <LibRiiEditor/ui/ThemeManager.hpp>
#include <LibRiiEditor/ui/Window.hpp>
#include "CoreContext.hpp"

struct Theme
{
	struct Editor
	{
		Editor() = default;
		~Editor() = default;

		const char* mTitle = "Theme Editor";

		void windowDraw(CoreContext& ctx) noexcept;
	};

	ThemeManager::BasicTheme mThemeSelection = ThemeManager::BasicTheme::ImDark;
	ThemeManager mThemeManager;
};
