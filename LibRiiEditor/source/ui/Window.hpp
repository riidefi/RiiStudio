#pragma once

#include "common.hpp"

struct SelectionManager;

struct TODO
{};

struct WindowContext
{
	WindowContext(SelectionManager& mgr, TODO& res)
		: selectionManager(mgr), core_resource(res){}
	~WindowContext() = default;

	SelectionManager& selectionManager;
	TODO& core_resource;
};

struct Window
{
	//! @brief The destructor.
	//!
	virtual ~Window() {}
	
	//! @brief Draw the window.
	//!
	//! @param[in] ctx Optional context (F.60, acceptable)
	//!
	virtual void draw(WindowContext* ctx = nullptr) noexcept = 0;

	bool bOpen = true;
	u32 mId; //!< ID, assigned by window manager, do not modify
};