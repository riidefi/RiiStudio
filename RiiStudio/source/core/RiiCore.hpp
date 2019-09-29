#pragma once

#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include "CoreContext.hpp"
#include "Theme.hpp"

class RiiCore : public Applet
{
public:

// TODO


	WindowContext makeWindowContext() override
	{
		return WindowContext(getSelectionManager(), mCoreRes);
	}

	void drawRoot() override;

private:
	DockSpace mDockSpace;
	Theme mTheme;
	EditorCoreRes mCoreRes = EditorCoreRes(mTheme);
};
