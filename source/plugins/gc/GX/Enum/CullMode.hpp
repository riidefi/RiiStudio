#pragma once

#include <core/common.h>

namespace libcube { namespace gx {

enum class CullMode : u32
{
	None,
	Front,
	Back,
	All
};

enum class DisplaySurface
{
	Both,
	Back,
	Front,
	Neither
};

inline DisplaySurface cullModeToDisplaySurfacee(CullMode c)
{
	return static_cast<DisplaySurface>(4 - static_cast<int>(c));
}

} } // namespace libcube::gx
