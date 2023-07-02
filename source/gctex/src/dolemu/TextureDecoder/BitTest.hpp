#pragma once

#include <stdint.h>
#if UINTPTR_MAX == 0xffffffff
#define ENVIRONMENT32
#elif UINTPTR_MAX == 0xffffffffffffffff
#define ENVIRONMENT64
#else
#error "Unsupported system"
#endif
