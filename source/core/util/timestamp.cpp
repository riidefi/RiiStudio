#include "timestamp.hpp"

#if defined(__VERSION__)
#ifdef __GNUC__
#define RII_CC "GCC " __VERSION__
#else
#define RII_CC __VERSION__
#endif
#elif defined(_MSC_VER)
#define STRINGIZE(x) STRINGIZE_(x)
#define STRINGIZE_(x) #x
#define RII_CC "MSVC " STRINGIZE(_MSC_VER)
#else
#define RII_CC "Unknown"
#endif

// Must match release on Github
#define GITHUB_TAG "Alpha 5.11.5"

const char GIT_TAG[] = GITHUB_TAG;

// Must be exactly 16 bytes long
const char VERSION_SHORT[] = "RiiStudioA5.11.5";
static_assert(sizeof(VERSION_SHORT) - 1 == 16,
              "VERSION_SHORT must be 16 bytes long");

#if defined(BUILD_DEBUG)
#define RII_BUILD GITHUB_TAG " Debug"
#elif defined(BUILD_RELEASE)
#define RII_BUILD GITHUB_TAG " Release"
#elif defined(BUILD_DIST)
#define RII_BUILD GITHUB_TAG
#else
#define RII_BUILD GITHUB_TAG " Custom"
#endif

extern "C" bool RII_IS_DIST_BUILD;
#if defined(BUILD_DIST)
bool RII_IS_DIST_BUILD = true;
#else
bool RII_IS_DIST_BUILD = false;
#endif

#if defined(BUILD_DIST) && defined(__EMSCRIPTEN__)
const char RII_TIME_STAMP[] = RII_BUILD;
#else
const char RII_TIME_STAMP[] =
    RII_BUILD " (Built " __DATE__ " at " __TIME__ ", " RII_CC ")";
#endif // defined(BUILD_DIST) && defined(__EMSCRIPTEN__)
