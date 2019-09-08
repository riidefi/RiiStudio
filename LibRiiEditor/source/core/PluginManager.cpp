#include "PluginManager.hpp"

template<typename T>
inline bool validatePluginSpan(const PluginRegistration::PluginSpan<T>& span)
{
	return span.data == 0 || span.data;
}

bool PluginManager::registerPlugin(const PluginRegistration& registration)
{
	if (registration.supported_extensions.size == 0 && registration.supported_magics.size == 0)
	{
		DebugReport("Warning: Plugin's domain is purely intensively determined.");
	}

	if (!validatePluginSpan(registration.supported_extensions) || validatePluginSpan(registration.supported_magics))
	{
		DebugReport("Invalid extension or magic arrays.");
		return false;
	}

	if (!registration.instrusive_check)
	{
		DebugReport("Extension has no intrusive check. This isn't optional.");
		return false;
	}

	if (!registration.plugin_name || !registration.plugin_version || !registration.plugin_domain)
	{
		DebugReport("Plugin is missing information (name/version/domain).");
		return false;
	}

	if (!registration.construct || !registration.destruct)
	{
		DebugReport("Plugin does not have complete constructor/destructor.");
		return false;
	}

	{
		std::lock_guard<std::mutex> guard(mMutex);

		const auto cur_idx = mPlugins.size();

		mPlugins.emplace_back(registration);

		if (registration.supported_extensions.size && registration.supported_extensions.data)
		{
			for (int i = 0; i < registration.supported_extensions.size; ++i)
				mExtensions.emplace(registration.supported_extensions.data[i], cur_idx);
		}

		if (registration.supported_magics.size && registration.supported_magics.data)
		{
			for (int i = 0; i < registration.supported_magics.size; ++i)
				mExtensions.emplace(registration.supported_magics.data[i], cur_idx);
		}
	}

	return true;
}
