#include "PluginFactory.hpp"


bool PluginFactory::registerPlugin(const pl::Package& package)
{
	if (package.mEditors.empty())
	{
		DebugReport("Plugin has no editors");
		return false;
	}

	for (int i = 0; i < package.mEditors.size(); ++i)
	{
		if (package.mEditors[i]->mExtensions.empty() == 0 && package.mEditors[i]->mMagics.empty())
		{
			DebugReport("Warning: Plugin's domain is purely intensively determined.");
			DebugReport("Intensive checking not yet supported, exiting...");
			return false;
		}

		// TODO: More validation

		{
			std::lock_guard<std::mutex> guard(mMutex);

			const auto cur_idx = mPlugins.size();

			mPlugins.push_back(package.mEditors[i]);

			const auto& ed = *package.mEditors[i];

			for (int j = 0; j < ed.mExtensions.size(); ++j)
				mExtensions.emplace_back(std::make_pair(std::string(ed.mExtensions[j]), cur_idx));

			for (int j = 0; j < ed.mMagics.size(); ++j)
				mMagics.emplace(ed.mMagics[j], cur_idx);
		}
	}
	return true;
}

std::unique_ptr<EditorWindow> PluginFactory::create(const std::string& extension, u32 magic)
{
	// TODO: Check extension
	
	const auto it = mMagics.find(magic);

	if (it != mMagics.end())
	{
		// TODO: Proceed to intensive check to verify match
		return std::make_unique<EditorWindow>(*mPlugins[it->second]);
	}

	// TODO: Perform intensive checking on all resources, pick most likely candidate
	throw "Unable to match plugin";
	assert(0);
	return nullptr;
}
