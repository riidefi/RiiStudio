#pragma once

#include "Exports.hpp"
#include <string>

#include <LibRiiEditor/pluginapi/Package.hpp>


namespace libcube {

pl::Package package()
{
	return pl::Package
	{
		// Package name
		{
			"LibCube",
			"libcube",
			"gc"
		},
		// States
		{

		},
		// Importers
		{

		}
	};
}


} // namespace libcube
