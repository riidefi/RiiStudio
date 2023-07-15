#ifndef AVIR_RS_H
#define AVIR_RS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void avir_resize(uint8_t* dst, uint32_t dst_size, uint32_t dx, uint32_t dy,
                 const uint8_t* src, uint32_t src_size, uint32_t sx,
                 uint32_t sy);
void clancir_resize(uint8_t* dst, uint32_t dst_size, uint32_t dx, uint32_t dy,
                    const uint8_t* src, uint32_t src_size, uint32_t sx,
                    uint32_t sy);

#ifdef __cplusplus
}
#endif

#endif /* AVIR_RS_H */
