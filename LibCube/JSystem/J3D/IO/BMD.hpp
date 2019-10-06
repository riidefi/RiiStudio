/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include <LibRiiEditor/common.hpp>
#include <LibCube/JSystem/J3D/Collection.hpp>
#include <LibRiiEditor/pluginapi/IO/Importer.hpp>

namespace libcube { namespace jsystem {

class BMDImporter : public pl::Importer
{
	bool tryRead(oishii::BinaryReader& reader, pl::FileState& state) override;
};

class BMDImporterSpawner : public pl::ImporterSpawner
{
	std::pair<MatchResult, std::string> match(const std::string& fileName, oishii::BinaryReader& reader) const override
	{
		u32 j3dMagic = reader.read<u32>();
		u32 bmdMagic = reader.read<u32>();
		reader.seek<oishii::Whence::Current>(-8);
		if (j3dMagic == 'J3D2' && (bmdMagic == 'bmd3' || bmdMagic == 'bdl4'))
		{
			return { MatchResult::Contents, "j3dcollection" };
		}
		return { MatchResult::Mismatch, "" };
	}
	std::unique_ptr<pl::Importer> spawn() const override
	{
		return std::make_unique<BMDImporter>();
	}
	std::unique_ptr<ImporterSpawner> clone() const override
	{
		return std::make_unique<BMDImporterSpawner>(*this);
	}

};



} } // namespace libcube::jsystem
