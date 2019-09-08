/*!
 * @file
 * @brief Messy C++ glue for the C-compatible plugin interface.
 */

#pragma once

#include "Plugin.hpp"

struct PluginWrapper : private Plugin
{
	PluginFactory& getFactory() { assert(mpPluginFactory); return *mpPluginFactory; }
};


// T must implement draw and static intrusiveCheck
template<typename T>
struct PluginWrapperInterface : public Plugin
{
	static void pl_draw(Plugin* pl, PluginContext* ctx)
	{
		static_cast<PluginWrapperInterface*>(pl)->wrapped_plugin.draw(*ctx);
	}
	PluginWrapperInterface()
	{
		plugin_draw = pl_draw;
	}

	static Plugin* interface_construct()
	{
		return new PluginWrapperInterface();
	}
	static void interface_destruct(Plugin* plugin)
	{
		delete (PluginWrapperInterface*)plugin;
	}

	static PluginRegistration createRegistration(
		const char* const* extensions, u32 nExtensions,
		const u32* magics, u32 nMagics,
		const char* name, const char* ver, const char* domain)
	{
		return PluginRegistration
		{
			{ extensions, nExtensions},
			{ magics, nMagics },
			T::intrusiveCheck,
			name,
			ver,
			domain,
			interface_construct,
			interface_destruct
		};
	}

	static PluginRegistration createRegistration(
		const std::vector<const char*>& extensions,
		const std::vector<u32>& magics,
		const std::string& name, const std::string& ver, const std::string& domain
	)
	{
		return createRegistration(&extensions[0], extensions.size(), &magics[0], magics.size(), name.c_str(), ver.c_str(), domain.c_str());
	}

	T wrapped_plugin;
};
