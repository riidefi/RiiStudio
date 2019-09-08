#pragma once

#include "ui/Window.hpp"
#include "Plugin.hpp"

struct PluginInstanceWrapper : private PluginInstance
{
	inline Plugin& getPlugin() noexcept
	{
		assert(mpPlugin);
		return *mpPlugin;
	}
	inline const Plugin& getPlugin() const noexcept
	{
		assert(mpPlugin);
		return *mpPlugin;
	}
	inline const PluginRegistration& getRegistration() const noexcept
	{
		return registration;
	}

	inline PluginInstanceWrapper(const PluginRegistration& regist) noexcept
		: PluginInstance(regist)
	{}
	inline ~PluginInstanceWrapper() noexcept = default;
};

//! @brief Interoperability between an applet window and a plugin instance.
//!
class PluginWindow : public Window, private PluginInstanceWrapper
{
	~PluginWindow() override = default;
	PluginWindow(const PluginRegistration& regist)
		: PluginInstanceWrapper(regist)
	{}

	//	void draw(WindowContext* ctx) noexcept override final
	//	{
	//		if (!ctx)
	//			return;
	//	}
};
