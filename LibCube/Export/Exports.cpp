#pragma once

#include "Exports.hpp"
#include <string>

#include <LibRiiEditor/pluginapi/Package.hpp>


namespace libcube {

Package::Package()
{
	mPackageName = {
		"LibCube",
		"libcube",
		"gc"
	};
	// States
	mEditors = {
		&mJ3DCollectionSpawner
	};

	// Importers
	mImporters = {

	};
}


} // namespace libcube
