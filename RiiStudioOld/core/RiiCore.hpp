#pragma once

#include <LibRiiEditor/core/Applet.hpp>
#include <LibRiiEditor/ui/widgets/Dockspace.hpp>
#include "CoreContext.hpp"
#include "Theme.hpp"
#include <vector>
#include <string>
#include <LibRiiEditor/core/PluginFactory.hpp>
#include "WindowAdaptor.hpp"
#include <queue>
#include <RiiStudio/editor/EditorWindow.hpp>
#include <RiiStudio/api/Interpreter.hpp>

struct Console;
class RiiCore : public Applet
{
public:
	enum class OpenFilePolicy
	{
		NewEditor,
		ReplaceEditorIfMatching,
		ReplaceEditor
	};

	WindowContext makeWindowContext() override
	{
		return WindowContext(getSelectionManager(), mCoreRes);
	}

	void drawRoot() override;
	void drawMenuBar();
	std::vector<std::string> fileDialogueOpen();
	void openFile(OpenFilePolicy policy = OpenFilePolicy::NewEditor);
	void openFile(const std::string& path, OpenFilePolicy policy = OpenFilePolicy::NewEditor);
	void save(const std::string& path);
	void saveAs();
	RiiCore();
	~RiiCore();

	void execute_python(const std::string& code)
	{
		mInterpreter.exec(code);
	}

	PluginFactory& getPluginFactory() { return mPluginFactory; }

private:
	DockSpace mDockSpace;
	Theme mTheme;
	EditorCoreRes mCoreRes = EditorCoreRes(mTheme);
	PluginFactory mPluginFactory;
	WindowAdaptor<Theme::Editor> mThemeEd;
	std::queue<pl::TransformStack::XFormContext> mTransformActions;

	void drawTransformsMenu(EditorWindow& window);
	void recursiveTransformsMenuItem(const std::string& type, EditorWindow& window);

	void handleTransformAction();

	Interpreter mInterpreter;
	std::unique_ptr<Console> mConsole;
};
