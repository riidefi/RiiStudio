#pragma once

/*!
 * @file
 * @brief Connects datablock nodes.
 */

#include "hook.hxx"

namespace oishii {

//! @brief Link between two endpoints (hooks). The linker will compute these.
//!
struct Link
{
	Hook from, to;
};


} // namespace DataBlock