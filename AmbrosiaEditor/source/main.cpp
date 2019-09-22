#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include <ui/ThemeManager.hpp>

#include <core/Plugin.hpp>
#include <core/PluginFactory.hpp>

#include "MOD/MOD.hpp"
#include "Export/Exports.hpp"

#include <fstream>

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
static inline std::string getOpenFileDialog(oishii::LPCSTR filter)
{
	using namespace oishii;
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	char szFile[MAX_PATH];

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST;
	GetOpenFileName(&ofn);

	return std::string(szFile, sizeof(szFile));
}
static bool openModelFile()
{
	std::string fileName = getOpenFileDialog("Pikmin 1 Model File (*.mod)\0*.mod\0");
	std::printf("Opening file %s\n", fileName.c_str());

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
		libcube::MOD modelFile;
		modelFile.read(reader);
	}

	fStream.close();
	return EXIT_SUCCESS;
}

class AmbrosiaEditor : public Applet
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
		auto editor = std::make_unique<AmbrosiaEditor>();

		plugin_factory->registerPlugin(libcube::PluginPackage);

		editor->attachWindow(plugin_factory->create(".tpl", 0x0020AF30));

		editor->frameLoop();
	}
}
