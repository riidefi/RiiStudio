#pragma once

#include <ui/ThemeManager.hpp>
#include <ui/Window.hpp>
#include "CoreContext.hpp"

class Theme
{
	static inline int toIntComp(float src)
	{
		return static_cast<int>(round(src * 255.0f));
	}
	static inline int toIntColor(const ImVec4& src)
	{
		return	(toIntComp(src.x) << 24) |
			(toIntComp(src.y) << 16) |
			(toIntComp(src.z) << 8) |
			(toIntComp(src.w));
	}

	ThemeManager::BasicTheme mThemeSelection = ThemeManager::BasicTheme::ImDark;
	ThemeManager mThemeManager;

	struct Editor
	{
		Editor() = default;
		~Editor() = default;

		const char* mTitle = "Theme Editor";

		void windowDraw(CoreContext& ctx) noexcept
		{
			auto& manager = ctx.core_resource.theme;

			ImGui::Combo("Theme", (int*)&manager.mThemeSelection, manager.mThemeManager.ThemeNames);

			if (manager.mThemeSelection == ThemeManager::BasicTheme::Adaptive)
			{
				ImVec4 bgColor = manager.mThemeManager.GetColor(manager.mThemeManager.BackGroundColor);
				ImVec4 txtColor = manager.mThemeManager.GetColor(manager.mThemeManager.TextColor);
				ImVec4 mainColor = manager.mThemeManager.GetColor(manager.mThemeManager.MainColor);
				ImVec4 accentColor = manager.mThemeManager.GetColor(manager.mThemeManager.MainAccentColor);
				ImVec4 highlightColor = manager.mThemeManager.GetColor(manager.mThemeManager.HighlightColor);

				ImGui::ColorEdit3("Background Color", &bgColor.x);
				ImGui::ColorEdit3("Text Color", &txtColor.x);
				ImGui::ColorEdit3("Main Color", &mainColor.x);
				ImGui::ColorEdit3("Accent Color", &accentColor.x);
				ImGui::ColorEdit3("Highlight Color", &highlightColor.x);

				manager.mThemeManager.SetColors(toIntColor(bgColor),
					toIntColor(txtColor),
					toIntColor(mainColor),
					toIntColor(accentColor),
					toIntColor(highlightColor));
			}
			manager.mThemeManager.setThemeEx(manager.mThemeSelection);
		}
	};
};
