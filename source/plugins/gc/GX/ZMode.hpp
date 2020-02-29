#pragma once

namespace libcube { namespace gx {

enum class Comparison;

struct ZMode
{
	bool compare;
	Comparison function;
	bool update;
};

} } // namespace libcube::gx
