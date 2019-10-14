#pragma once

#include "../Enum/Comparison.hpp"

namespace libcube { namespace gx {

struct ZMode
{
	bool compare = true;
	Comparison function = Comparison::LEQUAL;
	bool update = true;
};

} } // namespace libcube::gx
