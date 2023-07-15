#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

void impl_avir_resize(const uint8_t* src, uint32_t sx, uint32_t sy,
                      uint8_t* dst, uint32_t dx, uint32_t dy);
void impl_clancir_resize(const uint8_t* src, uint32_t sx, uint32_t sy,
                         uint8_t* dst, uint32_t dx, uint32_t dy);

#ifdef __cplusplus
}
#endif
