#pragma once

#include <stdint.h>
#include <vector>

namespace oishii {

class BreakpointHolder {
public:
  bool shouldBreak(uint32_t pos, uint32_t size);

  void add_bp(uint32_t offset, uint32_t size);
  template <typename U> void add_bp(uint32_t offset) {
    add_bp(offset, sizeof(U));
  }

private:
  struct BP {
    uint32_t offset{};
    uint32_t size{};
  };
  std::vector<BP> m_breakPoints;
};

} // namespace oishii
