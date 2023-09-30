#pragma once

#include <cstdio>
#include <iostream>
#include <rsl/DebugBreak.hpp>
#include <rsl/Types.hpp>
#include <set>
#include <span>
#include <stdint.h>
#include <vector>

namespace oishii {

enum class Whence {
  Set,     // Absolute -- start of file
  Current, // Relative -- current position
  End,     // Relative -- end position
};

} // namespace oishii
