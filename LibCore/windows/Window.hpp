#pragma once

#include <LibCore/common.h>

struct Window
{
	virtual void showMouse() {}
	virtual void hideMouse() {}

	//! @brief The destructor.
	//!
	virtual ~Window() {
		/* Propogate */ if (mParent && mParent != this && mParent->getActiveWindow() == this)
			mParent->setActiveWindow(nullptr);
	}
	
	//! @brief Draw the window.
	//!
	virtual void draw(Window* parent = nullptr) noexcept = 0;
    
// TODO...    
	bool bOpen = true; //!< If the window is opened, for ImGui. When false, window manager will destroy the window itself.
	u32 mId; //!< ID, assigned by window manager, do not modify
	u32 parentId = 0;


	void setActiveWindow(Window* win) { mActive = win; /* Propogate */ if (mParent && mParent != this) mParent->setActiveWindow(this); }
	Window* getActiveWindow() { return mActive; }
private:
	Window* mActive = nullptr;
protected:
	Window* mParent = nullptr;
};
