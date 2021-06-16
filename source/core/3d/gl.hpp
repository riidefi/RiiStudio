// Include order matters
#ifdef _WIN32
#include <GL/gl3w.h>
#define RII_GL
#elif defined(__EMSCRIPTEN__)
#include <EGL/egl.h>
#define RII_GL
#endif

#ifdef _WIN32
#include "glfw/glfw3.h"
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif
