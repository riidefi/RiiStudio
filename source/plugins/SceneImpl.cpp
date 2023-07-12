#include "SceneImpl.hpp"

namespace riistudio::lib3d {

// PUBLIC FUNCTION
glm::mat4 calcSrtMtxSimple(const lib3d::Bone& bone, const lib3d::Model* mdl) {
  if (mdl == nullptr)
    return {};

  return calcSrtMtxSimple(bone, mdl->getBones());
}

} // namespace riistudio::lib3d
