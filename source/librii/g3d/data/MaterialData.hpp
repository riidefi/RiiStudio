#pragma once

#include <core/common.h>

namespace librii::g3d {

// Note: In G3D this is per-material; in J3D it is per-texmatrix
enum class TexMatrixMode { Maya, XSI, Max };

enum class G3dMappingMode {
  Standard,
  EnvCamera,
  Projection,
  EnvLight,
  EnvSpec
};

enum class G3dIndMethod {
  Warp,
  NormalMap,
  SpecNormalMap,
  Fur,
  Res0,
  Res1,
  User0,
  User1
};

struct G3dIndConfig {
  G3dIndMethod method{G3dIndMethod::Warp};
  s8 normalMapLightRef{-1};

  bool operator==(const G3dIndConfig& rhs) const = default;
};

} // namespace librii::g3d
