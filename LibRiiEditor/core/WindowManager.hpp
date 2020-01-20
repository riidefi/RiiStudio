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
protected:
	u32 getMaxWindowIdCurrent() const { return mWindowIdCounter; }
	u32 getWindowIndexById(u32 id) const;
	// TODO: No lifetime guarantee
	Window& getWindowIndexed(u32 idx);
	u32 getNumWindows() const;
	bool bOnlyDrawActive = false;
private:
	SelectionManager mSelectionManager;
	WindowManagerInternal* intern; // Unfortunate hack required for C++/CLR (C# wrapper) to include this file -- no mutex support

	// Must only be accessed from processWindowQueue
	u32 mWindowIdCounter = 0; // Don't decrement

	int mActive = -1;
public:
	Window* getActiveEditor()
	{
		if (mActive < 0 || mActive >= getMaxWindowIdCurrent())
			return nullptr;
		auto idx = getWindowIndexById(mActive);
		if (idx == -1) return nullptr;
		return &getWindowIndexed(idx);
	}
	void setActiveEditor(Window& win)
	{
		mActive = win.mId;
	}
};

template <typename T>
struct TWindowManager : public WindowManager
{
	T* TGetActiveEditor(u32 id) { return (T*)getActiveEditor(); }
};
