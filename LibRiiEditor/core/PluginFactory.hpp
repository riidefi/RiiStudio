#pragma once

#include "LibRiiEditor/pluginapi/Plugin.hpp"
#include "Plugin.hpp"
#include <map>
#include <mutex>
#include <optional>

//! @brief Manages all applet plugins.
//!
class PluginFactory
{
public:
	PluginFactory() = default;
	~PluginFactory() = default;

	//! @brief	Attempt to register a plugin based on its registration details.
	//!
	//! @return If the operation was successful.
	//!
	bool registerPlugin(const pl::Package& package);

	struct SpawnedImporter
	{
		std::string fileStateId;
		std::unique_ptr<pl::Importer> importer;
	};

	std::optional<SpawnedImporter> spawnImporter(const std::string& filename, oishii::BinaryReader& reader);

private:
	std::mutex mMutex; //!< When performing write operations (registering a plugin)

	//! Other data here references by index -- be careful to maintain that.
	std::vector<std::unique_ptr<pl::FileStateSpawner>> mPlugins;
	std::vector<std::unique_ptr<pl::ImporterSpawner>> mImporters;

	// -- Deprecated --
	//	std::vector<std::pair<std::string, std::size_t>> mExtensions; //!< Maps extension string to mPlugins index.
	//	std::map<u32, std::size_t> mMagics;		//!< Maps file magic identifiers to mPlugins index.
};
