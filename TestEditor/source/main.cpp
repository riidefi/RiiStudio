#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include <ui/ThemeManager.hpp>

#include <core/Plugin.hpp>
#include <core/PluginFactory.hpp>
#include <core/PluginWrapper.hpp>


struct TestPlugin : public PluginWrapper
{
	~TestPlugin()
	{
		DebugReport("Goodbye from test plugin!");
	}
	void draw(const PluginContext& ctx)
	{
		if (ImGui::Begin(("Test Plugin (Window ID: " + std::to_string(ctx.window_data.mId) + ")").c_str(), &ctx.window_data.bOpen))
		{
			ImGui::Text("Hello from test plugin!");
		}
		ImGui::End();
	}
	static PluginRegistration::CheckResult intrusiveCheck()
	{
		return PluginRegistration::CheckResult::Maybe;
	}
};

struct TestPluginInterface : private PluginWrapperInterface<TestPlugin>
{
	static const std::string name, version, domain;
	static const std::vector<const char*> extensions;
	static const std::vector<u32> magics;

	static PluginRegistration getRegistration()
	{
		return createRegistration(extensions, magics, name, version, domain);
	}
};

const std::string TestPluginInterface::name = "Test Plugin";
const std::string TestPluginInterface::version = "1.0";
const std::string TestPluginInterface::domain = "Test Files";
const std::vector<const char*> TestPluginInterface::extensions = { ".test" };
const std::vector<u32> TestPluginInterface::magics = { 'TST0' };

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
	TestEditor()
	{
		const auto regist = TestPluginInterface::getRegistration();
		mPluginFactory.registerPlugin(regist);
		attachWindow(mPluginFactory.create("", 'TST0'));
	}

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
	PluginFactory mPluginFactory;
};

void main()
{
	auto editor = std::make_unique<TestEditor>();

	editor->frameLoop();
}
