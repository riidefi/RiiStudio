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
	void save(const std::string& path);
	void saveAs();
	RiiCore();
	~RiiCore();

private:
	DockSpace mDockSpace;
	Theme mTheme;
	EditorCoreRes mCoreRes = EditorCoreRes(mTheme);
	PluginFactory mPluginFactory;
	WindowAdaptor<Theme::Editor> mThemeEd;
#if 0
	void DEBUGwriteBmd(const std::string& path);
#endif
	std::queue<pl::TransformStack::XFormContext> mTransformActions;

	void drawTransformsMenu(EditorWindow& window);
	void recursiveTransformsMenuItem(const std::string& type, EditorWindow& window);

};
