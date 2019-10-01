#pragma once

#include <LibRiiEditor/core/Applet.hpp>
#include <LibRiiEditor/ui/Window.hpp>


struct Theme;

// With a simple applet, the core resource would be the current file itself.
// However, with the actual editor, the core resource needs to reflect the state of the editor itself and its plugin windows.
struct EditorCoreRes : public CoreResource
{
	Theme& theme;

	EditorCoreRes(Theme& t)
		: theme(t)
	{}
};
struct CoreContext
{
	CoreContext(WindowContext& src) :
		selectionManager(src.selectionManager),
		core_resource(static_cast<EditorCoreRes&>(src.core_resource))
	{}
	~CoreContext() = default;

	SelectionManager& selectionManager;
	EditorCoreRes& core_resource;
};
