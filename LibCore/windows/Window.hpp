#pragma once

#include <LibCore/common.h>

struct Window
{
	//! @brief The destructor.
	//!
	virtual ~Window() {}
	
	//! @brief Draw the window.
	//!
	virtual void draw(Window* parent = nullptr) noexcept = 0;
    
// TODO...    
	bool bOpen = true; //!< If the window is opened, for ImGui. When false, window manager will destroy the window itself.
	u32 mId; //!< ID, assigned by window manager, do not modify
	u32 parentId = 0;
};
