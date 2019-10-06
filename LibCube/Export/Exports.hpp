#pragma once

#include "Exports.hpp"

#include <LibRiiEditor/pluginapi/Package.hpp>
#include <LibCube/JSystem/J3D/Collection.hpp>
#include <LibCube/JSystem/J3D/IO/BMD.hpp>
namespace libcube {

struct Package : public pl::Package
{
	Package();

	jsystem::J3DCollectionSpawner mJ3DCollectionSpawner;

	jsystem::BMDImporterSpawner mBMDImporterSpawner;
};

} // namespace libcube
