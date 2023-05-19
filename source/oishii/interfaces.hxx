#pragma once

#include <stdint.h>
#include <cstdio>
#include <iostream>
#include <rsl/DebugBreak.hpp>
#include <set>
#include <span>
#include <vector>

namespace oishii {

enum class Whence {
  Set,     // Absolute -- start of file
  Current, // Relative -- current position
  End,     // Relative -- end position
};

} // namespace oishii
