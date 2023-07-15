#include "bindings.h"

#include "avir/avir.h"
#include "avir/lancir.h"

void impl_avir_resize(const uint8_t* src, uint32_t sx, uint32_t sy,
                      uint8_t* dst, uint32_t dx, uint32_t dy) {
  avir::CImageResizer<> Avir8BitImageResizer(8);
  // TODO: Allow more customization (args, k)
  Avir8BitImageResizer.resizeImage(src, sx, sy, 0, dst, dx, dy, 4, 0);
}

void impl_clancir_resize(const uint8_t* src, uint32_t sx, uint32_t sy,
                         uint8_t* dst, uint32_t dx, uint32_t dy) {
  avir::CLancIR AvirLanczos;
  AvirLanczos.resizeImage(src, sx, sy, 0, dst, dx, dy, 4, 0);
}
