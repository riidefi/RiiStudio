#pragma once

#include <memory>
#include <variant>
#include <LibRiiEditor/ui/Window.hpp>
#include "SelectionManager.hpp"

struct WindowManagerInternal;

class WindowManager
{
public:
	WindowManager();
	~WindowManager();

	void attachWindow(std::unique_ptr<Window> window);
	void detachWindow(u32 windowId);

	// TODO: No lifetime guarantee
	Window& getWindowIndexed(u32 idx);

	void processWindowQueue();
	void drawWindows(WindowContext* ctx = nullptr);

	SelectionManager& getSelectionManager()
	{
		return mSelectionManager;
	}
	const SelectionManager& getSelectionManager() const
	{
		return mSelectionManager;
	}

private:
	SelectionManager mSelectionManager;
	WindowManagerInternal* intern; // Unfortunate hack required for C++/CLR (C# wrapper) to include this file -- no mutex support

	// Must only be accessed from processWindowQueue
	u32 mWindowIdCounter = 0; // Don't decrement
};
