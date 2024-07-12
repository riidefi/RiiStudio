#include <version>

// [build] Undefined symbols for architecture arm64:
// [build]   "std::__1::__is_posix_terminal(__sFILE*)", referenced from:
// [build]       rsmeshopt::RingIterator<unsigned long>::RingIterator(unsigned long, std::__1::span<unsigned long const, 18446744073709551615ul>) in librsmeshopt.a(2e40c9e35e9506f4-RsMeshOpt.o)
// [build] ld: symbol(s) not found for architecture arm64
#if (defined(__cpp_lib_print) && __cpp_lib_print >= 202207L) && !defined(__APPLE__)
#include <print>
#define RSM_PRINT std::print
#else
#define RSM_PRINT(...)
#endif
