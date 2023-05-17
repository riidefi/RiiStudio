#include "AbstractStream.hxx"

namespace oishii {

void BreakpointHolder::breakPointProcess(u32 tell, u32 size) {
  if (tell > 200'000'000) {
    fprintf(stderr, "File size is astronomical");
    rsl::debug_break();
    abort();
  }
#ifndef NDEBUG
  for (const auto& bp : mBreakPoints) {
    if (tell >= bp.offset && tell + size <= bp.offset + bp.size) {
      printf("Writing to %04u (0x%04x) sized %u\n", tell, tell, size);
      // warnAt("Breakpoint hit", tell, tell + sizeof(T));
      rsl::debug_break();
    }
  }
#endif
}

void AbstractStream::breakPointProcess(u32 size) {
  BreakpointHolder::breakPointProcess(tell(), size);
}

} // namespace oishii
