#pragma once

#include "Exports.hpp"

#include <LibRiiEditor/pluginapi/Package.hpp>
#include <LibCube/JSystem/J3D/Collection.hpp>
#include <LibCube/JSystem/J3D/Binary/BMD/BMD.hpp>

namespace libcube {

struct GCCollectionRegistration : public pl::InterfaceRegistration
{
	GCCollectionRegistration()
	{
		mName = { "Gamecube 3D Data", "gc_collection", "gc_data" };
		mFlag = static_cast<u32>(pl::InterfaceFlag::Copyable);
		mMirror.emplace_back("gc_collection", "transform_stack", offsetof(GCCollection, mGcXfs));
	}
};

struct Package : public pl::Package
{
	Package();

	jsystem::J3DCollectionSpawner mJ3DCollectionSpawner;

	std::pair<jsystem::BMDImporterSpawner, jsystem::BMDExporterSpawner> mBMD;

	GCCollectionRegistration mGCCollection;
};

} // namespace libcube
