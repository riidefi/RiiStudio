#include "timestamp.hpp"

#if defined(__VERSION__)
#define RII_CC __VERSION__
#elif defined(_MSC_VER)
#define STRINGIZE(x) STRINGIZE_(x)
#define STRINGIZE_(x) #x
#define RII_CC "MSVC " STRINGIZE(_MSC_VER)
#else
#define RII_CC "Unknown"
#endif

// Must match release on Github
const char GIT_TAG[] = "Alpha 5.9 (Hotfix 4)";

// Must be exactly 16 bytes long
const char VERSION_SHORT[] = "RiiStudio: A-5.9";
static_assert(sizeof(VERSION_SHORT) - 1 == 16,
              "VERSION_SHORT must be 16 bytes long");

#if defined(BUILD_DEBUG)
#define RII_BUILD "Alpha Debug"
#elif defined(BUILD_RELEASE)
#define RII_BUILD "Alpha Release"
#elif defined(BUILD_DIST)
#define RII_BUILD "Alpha 5.9 (Hotfix 4)"
#else
#define RII_BUILD "Custom"
#endif

#if defined(BUILD_DIST) && defined(__EMSCRIPTEN__)
const char RII_TIME_STAMP[] = RII__BUILD;
#else
const char RII_TIME_STAMP[] =
    RII_BUILD " (Built " __DATE__ " at " __TIME__ ", " RII_CC ")";
#endif // defined(BUILD_DIST) && defined(__EMSCRIPTEN__)
