#pragma once

#include "Exports.hpp"
#include <string>

#include "Editors/TPLEditor.hpp"
#include "Editors/MODEditor.hpp"
#include "Editors/TXEEditor.hpp"
#include "Editors/BTIEditor.hpp"

namespace libcube {

pl::Package PluginPackage
{
	// Package name
	{
		"LibCube",
		"libcube",
		"gc"
	},
	// Editors
	{
		getTPLEditorHandle(),
		getMODEditorHandle(),
		getTXEEditorHandle(),
		getBTIEditorHandle()
	}
};

} // namespace libcube
