// Include order matters
#ifdef _WIN32
#include <GL/gl3w.h>
#elif defined(__EMSCRIPTEN__)
#include <EGL/egl.h>
#endif

#ifdef _WIN32
#include "glfw/glfw3.h"
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif
