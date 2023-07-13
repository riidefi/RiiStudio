#pragma once

#include "SceneState.hpp"
#include <glm/mat4x4.hpp>

namespace librii::gfx {

void PushCube(DrawBuffer& out, glm::mat4 modelMtx, glm::mat4 viewMtx,
              glm::mat4 projMtx);

} // namespace librii::gfx
