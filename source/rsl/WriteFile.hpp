#pragma once

#include <core/common.h>

namespace rsl {

[[nodiscard]] Result<void> WriteFile(const std::span<const uint8_t> data,
                                     const std::string_view path);

} // namespace rsl
