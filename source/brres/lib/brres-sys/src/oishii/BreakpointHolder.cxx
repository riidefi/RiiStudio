#include "BreakpointHolder.hxx"

namespace oishii {

void BreakpointHolder::add_bp(uint32_t offset, uint32_t size) {
  m_breakPoints.push_back(BP{
      .offset = offset,
      .size = size,
  });
}

bool BreakpointHolder::shouldBreak(uint32_t tell, uint32_t size) {
  for (const auto& bp : m_breakPoints) {
    if (tell >= bp.offset && tell + size <= bp.offset + bp.size) {
      return true;
    }
  }
  return false;
}

} // namespace oishii
