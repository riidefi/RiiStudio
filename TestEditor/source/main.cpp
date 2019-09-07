#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include <ui/ThemeManager.hpp>

static inline int toIntComp(float src)
{
	return static_cast<int>(round(src * 255.0f));
}
static inline int toIntColor(const ImVec4& src)
{
	return	(toIntComp(src.x) << 24) |
			(toIntComp(src.y) << 16) |
			(toIntComp(src.z) <<  8) |
			(toIntComp(src.w)	   ) ;
}

class TestEditor : public Applet
{
	WindowContext makeWindowContext() override
	{
		return WindowContext(getSelectionManager(), mCoreRes);
	}

	void drawRoot() override
	{
		mDockSpace.draw();

		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("Test"))
			ImGui::EndMenu();

		ImGui::EndMainMenuBar();
		ImGui::End();

		if (ImGui::Begin("Style Editor"))
		{
			ImGui::Combo("Theme", (int*)&mThemeSelection, mThemeManager.ThemeNames);

			if (mThemeSelection == ThemeManager::BasicTheme::Adaptive)
			{
				ImVec4 bgColor = mThemeManager.GetColor(mThemeManager.BackGroundColor);
				ImVec4 txtColor = mThemeManager.GetColor(mThemeManager.TextColor);
				ImVec4 mainColor = mThemeManager.GetColor(mThemeManager.MainColor);
				ImVec4 accentColor = mThemeManager.GetColor(mThemeManager.MainAccentColor);
				ImVec4 highlightColor = mThemeManager.GetColor(mThemeManager.HighlightColor);

				ImGui::ColorEdit3("Background Color", &bgColor.x);
				ImGui::ColorEdit3("Text Color", &txtColor.x);
				ImGui::ColorEdit3("Main Color", &mainColor.x);
				ImGui::ColorEdit3("Accent Color", &accentColor.x);
				ImGui::ColorEdit3("Highlight Color", &highlightColor.x);

				mThemeManager.SetColors(toIntColor(bgColor),
										toIntColor(txtColor),
										toIntColor(mainColor),
										toIntColor(accentColor),
										toIntColor(highlightColor));
			}
			mThemeManager.setThemeEx(mThemeSelection);
		}
		ImGui::End();

		ImGui::ShowDemoWindow();
	}

private:
	DockSpace mDockSpace;
	CoreResource mCoreRes;
	ThemeManager::BasicTheme mThemeSelection = ThemeManager::BasicTheme::Default;
	ThemeManager mThemeManager;
};

void WinMain()
{
	auto editor = std::make_unique<TestEditor>();

	editor->frameLoop();
}
