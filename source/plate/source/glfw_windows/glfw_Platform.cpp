#ifdef RII_BACKEND_GLFW

#include <cstdio>
#include <fstream>
#include <imgui/imgui.h>
#include <imgui/impl/imgui_impl_glfw.h>
#include <imgui/impl/imgui_impl_opengl3.h>
#include <plate/Platform.hpp>
#include <plate/gl.hpp>
#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <vector>

namespace plate {

static void handleGlfwError(int err, const char* description) {
  fprintf(stderr, "GLFW Error: %d: %s\n", err, description);
}

static void GlCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                       GLsizei length, const GLchar* message,
                       GLvoid* userParam) {
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    return;

  printf("[GL] %s\n", message);
}

inline void AttachGlDebugPrinter() {
  // TODO: Investigate enabling this on other platforms...
#ifdef _WIN32
  glDebugMessageCallback(GlCallback, 0);
#endif
}

static void handleDrop(GLFWwindow* window, int count, const char** raw_paths) {
  Platform* glwin =
      reinterpret_cast<Platform*>(glfwGetWindowUserPointer(window));
  if (!glwin || !count || !raw_paths)
    return;

  for (int i = 0; i < count; ++i)
    glwin->enqueueDrop(raw_paths[i]);
}

static bool initWindow(GLFWwindow*& pWin, int width = 1280, int height = 720,
                       const char* pName = "Untitled Window",
                       Platform* user = nullptr) {
  pWin = glfwCreateWindow(width, height, pName, NULL, NULL);
  if (!pWin)
    return false;

  glfwSetWindowUserPointer(pWin, user);
  glfwSetDropCallback(pWin, handleDrop);

  glfwMakeContextCurrent(pWin);

  glfwSwapInterval(1); // vsync
  return true;
}

static const char* glsl_version = "#version 100";

#define GL_VERSION_MAJOR 4
#define GL_VERSION_MINOR 1

Platform::Platform(unsigned width, unsigned height, const std::string& pName)
    : mTitle(pName) {
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW!\n");
    exit(1);
  }

  glfwSetErrorCallback(handleGlfwError);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef BUILD_DEBUG
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
  initWindow(*(GLFWwindow**)&mPlatformWindow, width, height, mTitle.c_str(),
             this);

  gl3wInit();

  // Print out GL errors
  AttachGlDebugPrinter();

  // Defer init..
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
                    ImGuiConfigFlags_DockingEnable |
                    ImGuiConfigFlags_ViewportsEnable;

  ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)mPlatformWindow, true);

  ImGui_ImplOpenGL3_Init(glsl_version);
}

void Platform::showMouse() {
  glfwSetInputMode((GLFWwindow*)mPlatformWindow, GLFW_CURSOR,
                   GLFW_CURSOR_NORMAL);
}
void Platform::hideMouse() {
  glfwSetInputMode((GLFWwindow*)mPlatformWindow, GLFW_CURSOR,
                   GLFW_CURSOR_DISABLED);
}
void Platform::setVsync(bool v) { glfwSwapInterval(v ? 1 : 0); }

Platform::~Platform() {
  ImGui_ImplGlfw_Shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow((GLFWwindow*)mPlatformWindow);
  glfwTerminate();
}

void Platform::enter() {
  const auto open_file = [&](const std::string& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);

    if (!stream)
      return;

    const auto size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(size);
    if (!stream.read(reinterpret_cast<char*>(data.get()), size))
      return;

    FileData drop{std::move(data), static_cast<std::size_t>(size), path};
    mDataDropQueue.emplace(std::move(drop));
  };

  while (!glfwWindowShouldClose((GLFWwindow*)mPlatformWindow)) {
    glfwPollEvents();

    while (!mDropQueue.empty()) {
      std::string to_open = mDropQueue.front();
      mDropQueue.pop();
      open_file(to_open);
    }
    while (!mDataDropQueue.empty()) {
      assert(!mDataDropQueue.front().mPath.empty() &&
             mDataDropQueue.front().mPath[0]);
      auto front = std::move(mDataDropQueue.front());
      vdropDirect(std::move(front.mData), front.mLen, front.mPath);
      mDataDropQueue.pop();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    rootCalc();

    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize((GLFWwindow*)mPlatformWindow, &display_w,
                           &display_h);
    glViewport(0, 0, display_w, display_h);
    // glClearColor(window.clear_color.x, window.clear_color.y,
    // window.clear_color.z, window.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    rootDraw();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
    glfwSwapBuffers((GLFWwindow*)mPlatformWindow);
  }
}

void Platform::writeFile(const std::span<uint8_t> data,
                         const std::string_view path) {
  std::ofstream stream(std::string(path), std::ios::binary | std::ios::out);
  stream.write(reinterpret_cast<const char*>(data.data()), data.size());
}

} // namespace plate

#endif // RII_BACKEND_GLFW
