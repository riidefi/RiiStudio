#pragma once

// Fine as its a PCH

#include <array>
#include <bitset>
#include <cassert>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <numbers> // pi
#include <numeric> // accumulate is here for some reason
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <rsl/Expect.hpp>
#include <rsl/Log.hpp>
#include <rsl/Pow2.hpp>
#include <rsl/Result.hpp>
#include <rsl/Round.hpp>
#include <rsl/Translate.hpp>
#include <rsl/Try.hpp>
#include <rsl/Types.hpp>

// When clang-cl `import std` support, switch to these and remove above includes
// from this header
#if 1
#define IMPORT_STD                                                             \
  namespace {}

#else
#define IMPORT_STD                                                             \
import std.core;                                                             \
import std.threading;                                                        \
import std.filesystem
#endif
