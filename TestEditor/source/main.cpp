/// Must include to properly build (tested in windows, vs2019)
//////////////////////////////////////////////////////////////////////
#if _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <future>
#endif
//////////////////////////////////////////////////////////////////////

#include <pfd/portable-file-dialogs.h>
#include <fstream>

#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include <ui/ThemeManager.hpp>

#include <core/Plugin.hpp>
#include <core/PluginFactory.hpp>

#include "MOD/MOD.hpp"
#include "Export/Exports.hpp"

namespace pk1 = libcube::pikmin1;
static bool openModelFile()
{
	auto selection = pfd::open_file("Select a file", ".", { "Pikmin 1 Model Files (*.mod)", "*.mod" }, false).result();
	if (selection.empty()) // user has pressed cancel or did not properly enter a file
		return EXIT_FAILURE;

	std::string fileName = selection[0];
	DebugReport("Opening file %s\n", fileName.c_str());

	std::ifstream fStream;
	fStream.open(fileName, std::ios::binary | std::ios::ate);

	if (!fStream.is_open())
		return EXIT_FAILURE;

	std::streamsize size = fStream.tellg();
	fStream.seekg(0, std::ios::beg);

	auto data = std::unique_ptr<char>(new char[static_cast<u32>(size)]);
	if (fStream.read(data.get(), size))
	{
		oishii::BinaryReader reader(std::move(data), static_cast<u32>(size), fileName.c_str());
		pk1::MOD modelFile;
		modelFile.read(reader);
	}

	fStream.close();
	return EXIT_SUCCESS;
}

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
				if (ImGui::MenuItem("Open..", "Ctrl+O")) { openModelFile(); }
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

void main()
{
	auto plugin_factory = std::make_unique<PluginFactory>();

	{
		auto editor = std::make_unique<TestEditor>();

		plugin_factory->registerPlugin(libcube::PluginPackage);

		editor->attachWindow(plugin_factory->create(".tpl", 0x0020AF30));

		editor->frameLoop();
	}
}
