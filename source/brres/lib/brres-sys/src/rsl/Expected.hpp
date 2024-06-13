#pragma once

#ifdef __APPLE__
#define RSL_USE_FALLBACK_EXPECTED
#endif

#ifdef RSL_USE_FALLBACK_EXPECTED
#include <tl/expected.hpp>
namespace std {
    using namespace tl;
}
#define RSL_UNEXPECTED tl::unexpected
#else
#include <expected>
#define RSL_UNEXPECTED std::unexpected
#endif
