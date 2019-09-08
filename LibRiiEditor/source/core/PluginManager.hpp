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

private:
	std::mutex					mMutex; //!< When performing write operations (registering a plugin)
	std::vector<PluginInstance> mPlugins; //!< Other data here references by index -- be careful to maintain that.
	std::map<std::string, u32>  mExtensions; //!< Maps extension string to mPlugins index.
	std::map<u32, u32>			mMagics; //!< Maps file magic identifiers to mPlugins index.
};
