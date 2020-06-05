#ifndef RII_TIME_STAMP

#if defined(__VERSION__)
#define __CC __VERSION__
#elif defined(_MSC_VER)
#define STRINGIZE(x) STRINGIZE_(x)
#define STRINGIZE_(x) #x
#define __CC "MSVC " STRINGIZE(_MSC_VER)
#else
#define __CC "Unknown"
#endif

#if defined(BUILD_DEBUG)
#define __BUILD "Alpha Debug"
#elif defined(BUILD_RELEASE)
#define __BUILD "Alpha Release"
#elif defined(BUILD_DIST)
#define __BUILD "Alpha" // TODO: better system for this
#else
#define __BUILD "Custom"
#endif

#define RII_TIME_STAMP __BUILD " (Built " __DATE__ " at " __TIME__ ", " __CC ")"
#endif // RII_TIME_STAMP