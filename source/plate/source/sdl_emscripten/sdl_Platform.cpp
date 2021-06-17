#ifdef RII_BACKEND_SDL

#include <plate/gl.hpp>

#include <cstdio>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <imgui/imgui.h>
#include <imgui/impl/imgui_impl_opengl3.h>
#include <imgui/impl/imgui_impl_sdl.h>
#include <plate/Platform.hpp>
#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <vector>

#include <SDL.h>
#include <SDL_opengles2.h>
#include <emscripten/bind.h>

// Emscripten requires to have full control over the main loop. We're going to
// store our SDL book-keeping variables globally. Having a single function that
// acts as a loop prevents us to store state in the stack of said function. So
// we need some location for this.
SDL_Window* g_Window = NULL;
SDL_GLContext g_GLContext = NULL;

bool gPointerLock = false;

namespace plate {

static Platform* g_AppWindow = nullptr;

void readFile(const int& addr, const size_t& len, std::string path) {
  uint8_t* data = reinterpret_cast<uint8_t*>(addr);
  printf("Data: %p\n", data);

  g_AppWindow->vdropDirect(std::unique_ptr<uint8_t[]>(data), len, path);
}

EMSCRIPTEN_BINDINGS(my_module) { emscripten::function("readFile", &readFile); }

void main_loop(void* arg) {
  reinterpret_cast<Platform*>(arg)->mainLoopInternal();
}

Platform::Platform(unsigned width, unsigned height, const std::string& pName)
    : mTitle(pName) {
  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    printf("Error: %s\n", SDL_GetError());
    return;
  }

  const char* glsl_version = "#version 100";
  // const char* glsl_version = "#version 300 es";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_DisplayMode current;
  SDL_GetCurrentDisplayMode(0, &current);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  static std::string sWinTitle = "App";
  if (!mTitle.empty())
    sWinTitle = mTitle;
  g_Window = SDL_CreateWindow(sWinTitle.c_str(), SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
  g_GLContext = SDL_GL_CreateContext(g_Window);

  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

  if (!g_GLContext) {
    fprintf(stderr, "Failed to initialize WebGL context!\n");
    return;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

  ImGui_ImplOpenGL3_Init(glsl_version);
  // For an Emscripten build we are disabling file-system access, so let's not
  // attempt to do a fopen() of the imgui.ini file. You may manually call
  // LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = NULL;

  SDL_GL_SetSwapInterval(1); // Enable vsync

  ImGui_ImplSDL2_InitForOpenGL(g_Window, g_GLContext);

  // Emscripten
  g_AppWindow = this;
}

void Platform::showMouse() { SDL_SetRelativeMouseMode(SDL_FALSE); }
void Platform::hideMouse() { SDL_SetRelativeMouseMode(SDL_TRUE); }
void Platform::setVsync(bool v) { SDL_GL_SetSwapInterval(v ? 1 : 0); }
Platform::~Platform() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void Platform::mainLoopInternal() {
  // Poll and handle events (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell
  // if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
  // your main application.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data
  // to your main application. Generally you may always pass all inputs to dear
  // imgui, and hide them from your application based on those two flags.
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    // Capture events here, based on io.WantCaptureMouse and
    // io.WantCaptureKeyboard
    switch (event.type) {
    // case SDL_DROPFILE: {
    //   vdrop({event.drop.file});
    //   break;
    // }
    default:
      break;
    }
  }
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(g_Window);
  ImGui::NewFrame();
  rootCalc();

  ImGui::Render();
  auto& io = ImGui::GetIO();

  SDL_GL_MakeCurrent(g_Window, g_GLContext);
  glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
  // glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
  rootDraw();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  SDL_GL_SwapWindow(g_Window);
}
// based on
// https://github.com/raysan5/raylib/commit/d2d4b17633d623999eb2509ff24f741b5d669b35#diff-cdfc1aea8847d91e401cad52d98e6d70R2723
static EM_BOOL EmscriptenMouseCallback(int eventType,
                                       const EmscriptenMouseEvent* mouseEvent,
                                       void* userData) {
  if (gPointerLock)
    emscripten_request_pointerlock(0, 1);
  else
    emscripten_exit_pointerlock();

  return 0;
}
void Platform::enter() {
  emscripten_set_main_loop_arg(&main_loop, (void*)this, 0, 0);
  // Hacky workaround...
  emscripten_set_click_callback(0, 0, 1, EmscriptenMouseCallback);
}

void Platform::writeFile(const std::span<uint8_t> data,
                         const std::string_view path) {
  static_assert(sizeof(void*) == sizeof(uint32_t), "emscripten pointer size");

  EM_ASM({ window.Module.downloadBuffer($0, $1, $2, $3); },
         reinterpret_cast<uint32_t>(data.data()), data.size(),
         reinterpret_cast<uint32_t>(path.data()), path.size());
}

} // namespace plate

#endif
