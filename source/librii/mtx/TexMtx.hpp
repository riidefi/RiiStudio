#pragma once

#include <core/common.h>
#include <glm/glm.hpp>

namespace librii::mtx {

enum class CommonMappingMethod {
  // Shared
  Standard,
  EnvironmentMapping,

  // J3D name. This is G3D's only PROJMAP.
  ViewProjectionMapping,

  // J3D only by default. EGG adds this to G3D as "ManualProjectionMapping"
  ProjectionMapping,
  ManualProjectionMapping = ProjectionMapping,

  // G3D
  EnvironmentLightMapping,
  EnvironmentSpecularMapping,

  // J3D only?
  ManualEnvironmentMapping // Specify effect matrix maunally
                           // J3D 4/5?
};

enum class CommonMappingOption {
  NoSelection,
  DontRemapTextureSpace, // -1 -> 1 (J3D "basic")
  KeepTranslation        // Don't reset translation column
};

enum class CommonTransformModel { Default, Maya, Max, XSI };

glm::mat4 computeTexSrt(const glm::vec2& scale, f32 rotate,
                        const glm::vec2& translate, CommonTransformModel xform);

glm::mat4 computeTexMtx(const glm::mat4& mdl, const glm::mat4& mvp,
                        const glm::mat4& texsrt, CommonMappingMethod method,
                        CommonMappingOption option);

} // namespace librii::mtx
