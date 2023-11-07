#pragma once

#include "SZS.hpp"
#include <algorithm>
#include <string.h>

namespace rlibrii::szs {

Result<std::vector<u8>> encodeCTGP(std::span<const u8> buf);

} // namespace rlibrii::szs
