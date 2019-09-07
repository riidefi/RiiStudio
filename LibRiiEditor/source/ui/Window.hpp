#pragma once

#include "common.hpp"

struct SelectionManager;


//! @brief		The core resource describes the root data (scene, archive, etc.) that an applet holds.
//!
//! @remarks	Management of the data itself is deferred to the applet implementation, requiring a potentially unsafe cast
//!				that should be safely encapsulated.
//!
struct CoreResource
{
	virtual ~CoreResource() = default;
};

//! @brief An (optional) context passed to windows when drawing.
//!
//! @remarks The core resource reference is non-owning -- the applet must not delete it while dispatching windows.
//!
struct WindowContext
{
	WindowContext(SelectionManager& mgr, CoreResource& res)
		: selectionManager(mgr), core_resource(res){}
	~WindowContext() = default;

	SelectionManager& selectionManager;
	CoreResource& core_resource;
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

	bool bOpen = true; //!< If the window is opened, for ImGui. When false, window manager will destroy the window itself.
	u32 mId; //!< ID, assigned by window manager, do not modify
};
