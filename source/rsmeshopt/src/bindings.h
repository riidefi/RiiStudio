#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 c_stripify(u32* dst, u32 algo, const u32* indices, u32 num_indices,
               const float* positions, u32 num_positions, u32 restart);

u32 c_makefans(u32* dst, const u32* indices, u32 num_indices, u32 restart,
               u32 min_len, u32 max_runs);

#ifdef __cplusplus
}
#endif
