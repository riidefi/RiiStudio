#pragma once

#include <core/3d/Bone.hpp>
#include <core/3d/Material.hpp>
#include <core/3d/Polygon.hpp>
#include <core/3d/Texture.hpp>
#include <librii/glhelper/VBOBuilder.hpp>
#include <memory> // std::shared_ptr

namespace libcube {
class Scene;
}

namespace riistudio::lib3d {

struct SceneState;
class Scene;

class DrawableDispatcher;

struct SceneBuffers;

} // namespace riistudio::lib3d

#include "Node.h"
