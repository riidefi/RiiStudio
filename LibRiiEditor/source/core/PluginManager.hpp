#pragma once

#include "Plugin.hpp"
#include <map>
#include <mutex>

//! @brief Manages all applet plugins.
//!
class PluginManager
{
public:
	PluginManager() = default;
	~PluginManager() = default;

	//! @brief	Attempt to register a plugin based on its registration details.
	//!
	//! @return If the operation was successful.
	//!
	bool registerPlugin(const PluginRegistration& registration);

	void instancePlugin(const PluginRegistration& registration)
	{
		mActivePlugins.emplace_back(registration);
	}

private:
	std::mutex						   mMutex;		//!< When performing write operations (registering a plugin)
	std::vector<PluginRegistration>	   mPlugins;	//!< Other data here references by index -- be careful to maintain that.
	std::map<std::string, std::size_t> mExtensions; //!< Maps extension string to mPlugins index.
	std::map<u32, std::size_t>		   mMagics;		//!< Maps file magic identifiers to mPlugins index.

	std::vector<PluginInstance>		   mActivePlugins;
};
