#pragma once

#ifdef __APPLE__
#include <tl/expected.hpp>
namespace std {
    using namespace tl;
}
#else
#include <expected>
#endif
