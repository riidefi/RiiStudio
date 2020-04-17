#pragma once

#include <core/common.h>

#include <core/3d/i3dmodel.hpp>

#include <memory>

#include <glm/mat4x4.hpp>

#include <core/kpi/Node.hpp>

namespace riistudio::frontend {

struct SceneState;

class Renderer {
public:
  Renderer();
  ~Renderer();
  void render(u32 width, u32 height, bool& hideCursor);
  void createShaderProgram() { bShadersLoaded = true; }
  void destroyShaders() { bShadersLoaded = false; }

  void prepare(const kpi::IDocumentNode& model,
               const kpi::IDocumentNode& texture, bool buf = true,
               bool tex = true);

private:
  bool bShadersLoaded = false;

  glm::vec3 max{0, 0, 0};
  std::vector<glm::vec3> pos;
  std::vector<u32> idx;
  std::vector<glm::vec3> colors;

  std::unique_ptr<SceneState> mState;

  // Mouse
  float mouseSpeed = 0.02f;
  // Input
  bool inCtrl = false;
  int combo_choice_cam = 0;
  // Camera settings
  float speed = 0.0f;
  float cmin = 1.0f;
  float cmax = 50000.f;
  float fov = 90.0f;
  // Camera state
  glm::vec3 eye{0.0f, 0.0f, 0.0f};
  glm::vec3 direction;
  // Render settings
  bool rend = true;
  bool wireframe = false;
};
} // namespace riistudio::frontend
