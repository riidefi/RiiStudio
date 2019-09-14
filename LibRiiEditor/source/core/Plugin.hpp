#pragma once

#include "common.hpp"

#if 0
#include "WindowManager.hpp"

#include <string>
#include <memory>

struct PluginFactory;
struct PluginContext
{
	WindowContext window_context;
	Window&	window_data;
	WindowManager& window_manager;
};

// New API: Plugin doesn't have a draw -- only if interface freely provides it\



struct PluginWindow : public PluginInstance, public WindowManager, public Window
{
	~PluginWindow() override = default;
	PluginWindow(const PluginRegistration& registration)
		: PluginInstance(registration)
	{}

	void draw(WindowContext* ctx) noexcept override final
	{
		if (!ctx)
			return;

		PluginContext pl_ctx{ *ctx, *static_cast<Window*>(this), *static_cast<WindowManager*>(this) };
		getPlugin().plugin_draw(&getPlugin(), &pl_ctx);
	}

};

#endif
