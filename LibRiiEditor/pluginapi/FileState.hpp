#pragma once

#include <vector>
#include <LibRiiEditor/pluginapi/RichName.hpp>

namespace pl {

struct AbstractInterface;

//! A concrete state in memory that can blueprint an editor.
//! IO must be handled by a separate endpoint
//! Undecided on supporting merging with io.
//!
struct FileState
{
	virtual ~FileState() = default;
	FileState() = default;

	RichName mName;
};

} // namespace pl
