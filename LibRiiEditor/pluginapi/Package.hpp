#pragma once

#include "RichName.hpp"
#include <vector>
#include <memory>

#include "FileStateSpawner.hpp"
#include "io/Importer.hpp"
#include "IO/Exporter.hpp"

namespace pl {

struct Package
{
	RichName mPackageName; // Command name unused

	std::vector<const FileStateSpawner*> mEditors; // point to member of class
	std::vector<const ImporterSpawner*> mImporters;
	std::vector<const ExporterSpawner*> mExporters;
};

} // namespace pl
