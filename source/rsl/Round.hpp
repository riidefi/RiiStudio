#pragma once

#include <stdint.h>

constexpr uint32_t roundDown(uint32_t in, uint32_t align) {
  return align ? in & ~(align - 1) : in;
}
constexpr uint32_t roundUp(uint32_t in, uint32_t align) {
  return align ? roundDown(in + (align - 1), align) : in;
}
