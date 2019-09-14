#include "PluginFactory.hpp"

template<typename T>
bool validatePluginSpan(const pl::Span<T>& sp)
{
	return sp.getNum() == 0 || sp.mpEntries;
}

bool PluginFactory::registerPlugin(const pl::Package& package)
{
	if (package.getEditors().getNum() == 0)
	{
		DebugReport("Plugin has no editors");
		return false;
	}

	for(int i = 0; i < package.getEditors().getNum(); ++i)
	{
		const pl::FileEditor& ed = package.getEditors()[i];


		if (ed.getExtensions().getNum() == 0 && ed.getMagics().getNum() == 0)
		{
			DebugReport("Warning: Plugin's domain is purely intensively determined.");
			DebugReport("Intensive checking not yet supported, exiting...");
			return false;
		}

		if (!validatePluginSpan(ed.getExtensions()) || !validatePluginSpan(ed.getMagics()))
		{
			DebugReport("Invalid extension or magic arrays.");
			return false;
		}

		// TODO: More validation

		{
			std::lock_guard<std::mutex> guard(mMutex);

			const auto cur_idx = mPlugins.size();

			mPlugins.emplace_back(ed);

			for (int j = 0; j < ed.getExtensions().getNum(); ++j)
				mExtensions.emplace_back(std::make_pair(std::string(ed.getExtensions()[j]), cur_idx));

			for (int j = 0; j < ed.getMagics().getNum(); ++j)
				mMagics.emplace(ed.getMagics()[j], cur_idx);
		}
	}
	return true;
}

std::unique_ptr<pl::EditorWindow> PluginFactory::create(const std::string& extension, u32 magic)
{
	// TODO: Check extension
	
	const auto it = mMagics.find(magic);

	if (it != mMagics.end())
	{
		// TODO: Proceed to intensive check to verify match
		return std::make_unique<pl::EditorWindow>(mPlugins[it->second]);
	}

	// TODO: Perform intensive checking on all resources, pick most likely candidate
	throw "Unable to match plugin";
	assert(0);
	return nullptr;
}
