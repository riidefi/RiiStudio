#pragma once

#include <vector>
#include <memory>

#include "RichName.hpp"
#include "Reflection.hpp"


namespace oishii { class BinaryReader; }

namespace pl {

struct FileState;

//! Constructs a file state pointer for use in runtime polymorphic code.
//!
struct FileStateSpawner
{
	virtual ~FileStateSpawner() = default;

	virtual std::unique_ptr<FileState> spawn() const = 0;
	virtual std::unique_ptr<FileStateSpawner> clone() const = 0;

	RichName mId;
	std::vector<MirrorEntry> mMirror;

	void addMirror(const std::string_view n, const std::string_view a, int t)
	{
		mMirror.push_back({ n, a, t });
	}
};

} // namespace pl
