#pragma once

#include <vector>
#include <string>
#include <oishii/reader/binary_reader.hxx>

namespace pl {

struct FileState;

//! No longer an interface, but a separate node in the data mesh.
//!
struct Importer
{
	virtual ~Importer() = default;

	virtual bool tryRead(oishii::BinaryReader& reader, FileState& state) = 0;
};
//	-- Deprecated for now
//	struct Options
//	{
//		std::vector<std::string> mExtensions;
//		std::vector<u32> mMagics;
//	} mOptions;
struct ImporterSpawner
{
	// EFE inspired
	enum class MatchResult
	{
		Magic,
		Contents,
		Mismatch
	};


	virtual ~ImporterSpawner() = default;
	// MatchResult, pinId
	virtual std::pair<MatchResult, std::string> match(const std::string& fileName, oishii::BinaryReader& reader) const = 0;
	virtual std::unique_ptr<Importer> spawn() const = 0;
	virtual std::unique_ptr<ImporterSpawner> clone() const = 0;
};

// TODO: Adapter
//	interface AbstractImporter
//	{
//		Importer::Options mOptions;
//	
//		// Where TState inherits FileState
//		bool import(oishii::BinaryReader& reader, TState& state);
//	};

} // namespace pl
