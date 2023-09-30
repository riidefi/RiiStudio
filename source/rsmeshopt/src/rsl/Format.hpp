// Copied from RiiStudio

#pragma once

#if defined(__APPLE__) || defined(__GCC__) || defined(__GNUC__)
#include <fmt/format.h>
namespace std {
using namespace fmt;
}
#else
#include <format>
#endif
