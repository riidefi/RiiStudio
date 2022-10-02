#include "timestamp.hpp"

#if defined(__VERSION__)
#define __CC __VERSION__
#elif defined(_MSC_VER)
#define STRINGIZE(x) STRINGIZE_(x)
#define STRINGIZE_(x) #x
#define __CC "MSVC " STRINGIZE(_MSC_VER)
#else
#define __CC "Unknown"
#endif

// Must match release on Github
const char GIT_TAG[] = "Alpha 5.4.5";

// Must be exactly 16 bytes long
const char VERSION_SHORT[] = "RiiStudio: A-5.4";
static_assert(sizeof(VERSION_SHORT) - 1 == 16,
              "VERSION_SHORT must be 16 bytes long");

#if defined(BUILD_DEBUG)
#define __BUILD "Alpha Debug"
#elif defined(BUILD_RELEASE)
#define __BUILD "Alpha Release"
#elif defined(BUILD_DIST)
#define __BUILD "Alpha 5.4.5"
#else
#define __BUILD "Custom"
#endif

#if defined(BUILD_DIST) && defined(__EMSCRIPTEN__)
const char RII_TIME_STAMP[] = __BUILD;
#else
const char RII_TIME_STAMP[] =
    __BUILD " (Built " __DATE__ " at " __TIME__ ", " __CC ")";
#endif // defined(BUILD_DIST) && defined(__EMSCRIPTEN__)
