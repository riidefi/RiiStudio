/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include <LibRiiEditor/common.hpp>
#include <LibCube/JSystem/J3D/Collection.hpp>
#include <LibRiiEditor/pluginapi/IO/Importer.hpp>
#include <LibRiiEditor/pluginapi/IO/Exporter.hpp>

namespace libcube::jsystem {

class BMDImporter : public pl::Importer
{
public:
	bool error = false;

	struct BMDOutputContext
	{
		J3DModel& mdl;

		// Compression ID LUT (remap table)
		std::vector<u16> jointIdLut;
		std::vector<u16> materialIdLut;
	};

	// Associate section magics with file positions and size
	struct SectionEntry
	{
		std::size_t streamPos;
		u32 size;
	};
	std::map<u32, SectionEntry> mSections;

	void lex(oishii::BinaryReader& reader, u32 sec_count) noexcept;


	void readBMD(oishii::BinaryReader& reader, BMDOutputContext& ctx);
	bool tryRead(oishii::BinaryReader& reader, pl::FileState& state) override;

private:
	void readDrawMatrices(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept;
	void readInformation(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept;
	void readJoints(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept;
	void readMaterials(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept;
	void readVertexBuffers(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept;
	bool enterSection(oishii::BinaryReader& reader, u32 id);
};

class BMDExporter : public pl::Exporter
{
public:
	~BMDExporter() = default;

	bool write(oishii::Writer& writer, pl::FileState& state) override;
};
class BMDExporterSpawner : public pl::ExporterSpawner
{
	bool match(const std::string& id) override
	{
		return id == "j3dcollection";
	}
	std::unique_ptr<pl::Exporter> spawn() const override
	{
		return std::make_unique<BMDExporter>();
	}
	std::unique_ptr<pl::ExporterSpawner> clone() const override
	{
		return std::make_unique<BMDExporterSpawner>(*this);
	}
};
class BMDImporterSpawner : public pl::ImporterSpawner
{
	std::pair<MatchResult, std::string> match(const std::string& fileName, oishii::BinaryReader& reader) const override
	{
		u32 j3dMagic = reader.read<u32, oishii::EndianSelect::Big>();
		u32 bmdMagic = reader.read<u32, oishii::EndianSelect::Big>();
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



} // namespace libcube::jsystem
