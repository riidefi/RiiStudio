#include <version>

#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
#include <print>
#define RSM_PRINT std::print
#else
#define RSM_PRINT(...)
#endif
