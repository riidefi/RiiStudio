#pragma once

#include <core/common.h>
#include <core/3d/i3dmodel.hpp>
#include <memory>
#include <glm/mat4x4.hpp>
#include <core/kpi/Node.hpp>
#include <frontend/editor/renderer/Camera.hpp>

namespace riistudio::frontend {

struct SceneState;

class Renderer {
public:
  Renderer();
  ~Renderer();
  void render(u32 width, u32 height, bool& hideCursor);

  void prepare(const kpi::IDocumentNode& model,
               const kpi::IDocumentNode& texture, bool buf = true,
               bool tex = true);

private:

  glm::vec3 max{0, 0, 0};
  std::vector<glm::vec3> pos;
  std::vector<u32> idx;
  std::vector<glm::vec3> colors;

  std::unique_ptr<SceneState> mState;


  Camera mCamera;

  // Mouse
  float mouseSpeed = 0.2f;
  // Input
  bool inCtrl = false;
  int combo_choice_cam = 0;
  // Render settings
  bool rend = true;
  bool wireframe = false;
};
} // namespace riistudio::frontend
