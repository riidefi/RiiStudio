#pragma once

#include <stdint.h>

constexpr bool is_power_of_2(uint32_t x) { return (x & (x - 1)) == 0; }
