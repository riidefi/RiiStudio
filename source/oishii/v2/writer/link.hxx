#pragma once

/*!
 * @file
 * @brief Connects datablock nodes.
 */

#include "hook.hxx"

namespace oishii::v2 {

//! @brief Link between two endpoints (hooks). The linker will compute these.
//!
struct Link
{
	Hook from, to;
	int mStride = 1;
};


} // namespace DataBlock
