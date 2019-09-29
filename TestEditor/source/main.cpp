#include "OpenFileFormats.hpp"

#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include <ui/ThemeManager.hpp>

#include <core/Plugin.hpp>
#include <core/PluginFactory.hpp>

#include "Export/Exports.hpp"

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
class TestEditor : public Applet
{
public:
	WindowContext makeWindowContext() override
	{
		return WindowContext(getSelectionManager(), mCoreRes);
	}

	void drawRoot() override
	{
		mDockSpace.draw();

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open MOD file", "")) { libcube::openMODFile(); }
				else if (ImGui::MenuItem("Open BTI file", "")) { libcube::openBTIFile(); }
				else if (ImGui::MenuItem("Open TXE file", "")) { libcube::openTXEFile(); }
				else if (ImGui::MenuItem("Open DCA file", "")) { libcube::openDCAFile(); }
				else if (ImGui::MenuItem("Open DCK file", "")) { libcube::openDCKFile(); }
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::End();

		if (ImGui::Begin("Style Editor"))
		{
			ImGui::Combo("Theme", (int*)& mThemeSelection, mThemeManager.ThemeNames);

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
	}

private:
	DockSpace mDockSpace;
	CoreResource mCoreRes;
	ThemeManager::BasicTheme mThemeSelection = ThemeManager::BasicTheme::ImDark;
	ThemeManager mThemeManager;
};

int main(int argc, char* const* argv)
{
	auto plugin_factory = std::make_unique<PluginFactory>();

	{
		auto editor = std::make_unique<TestEditor>();

		plugin_factory->registerPlugin(libcube::PluginPackage);

		editor->attachWindow(plugin_factory->create(".mod", 0x00000000));
		editor->attachWindow(plugin_factory->create(".tpl", 0x0020AF30));
		editor->attachWindow(plugin_factory->create(".txe", 0x00000001));
		editor->attachWindow(plugin_factory->create(".bti", 0x00000002));

		editor->frameLoop();

	}
	return EXIT_SUCCESS;
}
