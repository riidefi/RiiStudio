#pragma once

#include "RichName.hpp"
#include <vector>
#include <memory>

#include "FileStateSpawner.hpp"
#include "io/Importer.hpp"
#include "IO/Exporter.hpp"
#include "Reflection.hpp"

namespace pl {

enum class InterfaceFlag
{
	None,
	Copyable
};

// The plugin needs to share info about interfaces for the rest of the system to interact with it.
struct InterfaceRegistration
{
	RichName mName;
	u32 mFlag;


	// Declare the parents of the interface (really anything could be registered here)
	std::vector<MirrorEntry> mMirror;

};

struct Package
{
	RichName mPackageName; // Command name unused

	std::vector<const FileStateSpawner*> mEditors; // point to member of class
	// An importer/exporter transfers data from the disc.
	// It can be seen as the connection between a disc file and a file state
	std::vector<const ImporterSpawner*> mImporters;
	std::vector<const ExporterSpawner*> mExporters;

	// Reflectable classes:
	// FileStates (registered in FileStateSpawner)
	// Interfaecs (registered in InterfaceRegistration)
	std::vector<const InterfaceRegistration*> mInterfaces;
};

} // namespace pl
