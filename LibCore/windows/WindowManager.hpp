#pragma once

#include "Window.hpp"
#include <memory>

struct WindowManagerInternal;

class WindowManager : public Window
{
public:
    WindowManager();
    ~WindowManager();

    // Return the window ID
    u32 attachWindow(std::unique_ptr<Window> window);
    void detachWindow(u32 windowId);

    void processWindowQueue();
	void drawChildren(Window* ctx = nullptr) noexcept;

    u32 getWindowIndexById(u32 id) const;
	// TODO: No lifetime guarantee
	Window& getWindowIndexed(u32 idx);
	u32 getNumWindows() const;
	
private:
    std::unique_ptr<WindowManagerInternal> mInternal;
};
