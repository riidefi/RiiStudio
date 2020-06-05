// Include order matters
#ifdef _WIN32
#include <GL/gl3w.h>
#else
#include <EGL/egl.h>
#endif

#ifdef _WIN32
#include "glfw/glfw3.h"
#else
#include <GLES3/gl3.h>
#endif
