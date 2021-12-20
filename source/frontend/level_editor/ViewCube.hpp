#pragma once

#include <core/common.h>
#include <glm/mat4x4.hpp>

namespace riistudio::lvl {

bool DrawViewCube(float last_width, float last_height, glm::mat4& view_mtx,
                  u32 viewcube_bg = 0x0000'0000);

} // namespace riistudio::lvl
