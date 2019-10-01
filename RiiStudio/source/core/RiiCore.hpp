#pragma once

#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include "CoreContext.hpp"
#include "Theme.hpp"
#include <vector>
#include <string>

class RiiCore : public Applet
{
public:

// TODO


	WindowContext makeWindowContext() override
	{
		return WindowContext(getSelectionManager(), mCoreRes);
	}

	void drawRoot() override;

	void drawMenuBar();

	std::vector<std::string> fileDialogueOpen();

	enum class OpenFilePolicy
	{
		NewEditor,
		ReplaceEditorIfMatching,
		ReplaceEditor
	};
	void openFile(OpenFilePolicy policy = OpenFilePolicy::NewEditor);

private:
	DockSpace mDockSpace;
	Theme mTheme;
	EditorCoreRes mCoreRes = EditorCoreRes(mTheme);
};
