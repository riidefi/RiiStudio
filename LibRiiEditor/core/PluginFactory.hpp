#pragma once

#include "LibRiiEditor/pluginapi/Plugin.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <vector>

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
	// FIXME
	std::unique_ptr<pl::Exporter> spawnExporter(const std::string& id)
	{
		std::lock_guard g(mMutex);

		for (const auto& it : mExporters)
			if (it->match(id))
				return it->spawn();

		return nullptr;
	}
	std::unique_ptr<pl::FileState> spawnFileState(const std::string& fileStateId);

	// Install a DLL
	bool installModule(const std::string& path);

private:
	std::mutex mMutex; //!< When performing write operations (registering a plugin)

	//! Other data here references by index -- be careful to maintain that.
	std::vector<std::unique_ptr<pl::FileStateSpawner>> mPlugins;
	std::vector<std::unique_ptr<pl::ImporterSpawner>> mImporters;
	std::vector<std::unique_ptr<pl::ExporterSpawner>> mExporters;

	// -- Deprecated --
	//	std::vector<std::pair<std::string, std::size_t>> mExtensions; //!< Maps extension string to mPlugins index.
	//	std::map<u32, std::size_t> mMagics;		//!< Maps file magic identifiers to mPlugins index.
};
