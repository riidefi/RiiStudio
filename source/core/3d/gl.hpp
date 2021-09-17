#include <core/common.h>

// Include order matters
#if defined(RII_BACKEND_GLFW)
#include <GL/gl3w.h>
#define RII_GL
#elif defined(RII_BACKEND_SDL)
#include <EGL/egl.h>
#define RII_GL
#endif

#if defined(RII_BACKEND_GLFW)
#include "glfw/glfw3.h"
#elif defined(RII_BACKEND_SDL)
#include <GLES3/gl3.h>
#endif
