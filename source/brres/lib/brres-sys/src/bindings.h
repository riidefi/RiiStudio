#include "util.h"

#ifdef __EMSCRIPTEN__
#define WASM_EXPORT __attribute__((visibility("default")))
#else
#define WASM_EXPORT
#endif
