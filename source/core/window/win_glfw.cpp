// WIN32-GLFW
#ifdef RII_BACKEND_GLFW

#include "gl_window.hpp"
#include <core/3d/gl.hpp>
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <vector>
#include <vendor/glfw/glfw3.h>
#include <vendor/imgui/imgui.h>
#include <vendor/imgui/impl/imgui_impl_glfw.h>
#include <vendor/imgui/impl/imgui_impl_opengl3.h>

namespace riistudio::core {

static GLWindow* g_AppWindow = nullptr;
static void handleGlfwError(int err, const char* description) {
  fprintf(stderr, "GLFW Error: %d: %s\n", err, description);
}

static void handleDrop(GLFWwindow* window, int count, const char** raw_paths) {
  GLWindow* glwin =
      reinterpret_cast<GLWindow*>(glfwGetWindowUserPointer(window));
  if (!glwin || !count || !raw_paths)
    return;

  std::vector<std::string> paths;
  for (int i = 0; i < count; ++i)
    paths.emplace_back(raw_paths[i]);

  glwin->vdrop(paths);
}

static bool initWindow(GLFWwindow*& pWin, int width = 1280, int height = 720,
                       const char* pName = "Untitled Window",
                       GLWindow* user = nullptr) {
  pWin = glfwCreateWindow(width, height, pName, NULL, NULL);
  if (!pWin)
    return false;

  glfwSetWindowUserPointer(pWin, user);
  glfwSetDropCallback(pWin, handleDrop);

  glfwMakeContextCurrent(pWin);

  glfwSwapInterval(1); // vsync
  return true;
}

static const char* glsl_version = "#version 130";

GLWindow::GLWindow(int width, int height, const std::string& pName)
    : mTitle(pName) {
  
  if (glfwInit()) {
    glfwSetErrorCallback(handleGlfwError);

#if 0 // Old GL version
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
    // SDL_GL_CONTEXT_PROFILE_ES);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
    initWindow(mGlfwWindow, width, height, mTitle.c_str(), this);

    gl3wInit();

    // Defer init..
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
                      ImGuiConfigFlags_DockingEnable |
                      ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForOpenGL(getGlfwWindow(), true);

    ImGui_ImplOpenGL3_Init(glsl_version);
    return;
  }

  fprintf(stderr, "Failed to initialize GLFW!\n");
  exit(1);
}

void GLWindow::glfwshowMouse() {
  glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
void GLWindow::glfwhideMouse() {
  glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void GLWindow::setVsync(bool v) { glfwSwapInterval(v ? 1 : 0); }
GLWindow::~GLWindow() {
  glfwDestroyWindow(mGlfwWindow);
  glfwTerminate(); // TODO: Move out

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void GLWindow::mainLoopInternal() {
  glfwPollEvents();

  frameProcess();

  int display_w, display_h;
  glfwGetFramebufferSize(mGlfwWindow, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  // glClearColor(window.clear_color.x, window.clear_color.y,
  // window.clear_color.z, window.clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  frameRender();

  // Update and Render additional Platform Windows
  // (Platform functions may change the current OpenGL context, so we
  // save/restore it to make it easier to paste this code elsewhere.
  //  For this specific demo app we could also call
  //  glfwMakeContextCurrent(window) directly)
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow* backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }
  glfwSwapBuffers(mGlfwWindow);
}
void GLWindow::loop() {
  while (!glfwWindowShouldClose(mGlfwWindow)) {
    mainLoopInternal();
  }
}
void GLWindow::newFrame() { ImGui_ImplGlfw_NewFrame(); }

} // namespace riistudio::core

#endif // RII_BACKEND_GLFW
