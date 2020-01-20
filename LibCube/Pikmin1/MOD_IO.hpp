/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include <LibRiiEditor/common.hpp>
#include "Pikmin1Model.hpp"
#include <LibRiiEditor/pluginapi/IO/Importer.hpp>

namespace libcube { namespace pikmin1 {

class MODImporter : public pl::Importer
{
	enum class Chunks : u16
	{
		Header = 0x0000,

		VertexPosition = 0x0010,
		VertexNormal = 0x0011,
		VertexNBT = 0x0012,
		VertexColor = 0x0013,

		VertexUV0 = 0x0018,
		VertexUV1 = 0x0019,
		VertexUV2 = 0x001A,
		VertexUV3 = 0x001B,
		VertexUV4 = 0x001C,
		VertexUV5 = 0x001D,
		VertexUV6 = 0x001E,
		VertexUV7 = 0x001F,

		Texture = 0x0020,
		TextureAttribute = 0x0022,
		Material = 0x0030,

		VertexMatrix = 0x0040,

		Envelope = 0x0041,

		Mesh = 0x0050,

		Joint = 0x0060,
		JointName = 0x0061,

		CollisionPrism = 0x0100,
		CollisionGrid = 0x0110,

		EoF = 0xFFFF
	};
	bool tryRead(oishii::BinaryReader& reader, pl::FileState& state) override;
	void readHeader(oishii::BinaryReader& bReader, Model& mdl);
	void readCollisionPrism(oishii::BinaryReader& bReader, Model& mdl);
};

class MODImporterSpawner : public pl::ImporterSpawner
{
	std::pair<MatchResult, std::string> match(const std::string& fileName, oishii::BinaryReader& reader) const override
	{
		// Unfortunately no magic -- just compare extension for now.

		const std::string ending = ".mod";

		if (fileName.size() > 4 && std::equal(ending.rbegin(), ending.rend(), fileName.rbegin()))
		{
			return { MatchResult::Contents, "pikmin1mod" };
		}
		return { MatchResult::Mismatch, "" };
	}
	std::unique_ptr<pl::Importer> spawn() const override
	{
		return std::unique_ptr<pl::Importer>(new MODImporter());
	}
	std::unique_ptr<ImporterSpawner> clone() const override
	{
		return std::make_unique<MODImporterSpawner>();
	}

};

} } // namespace libcube::pikmin1
