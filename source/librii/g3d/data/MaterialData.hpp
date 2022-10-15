#pragma once

#include <core/common.h>
#include <librii/gx.h>

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

// This doesn't exist outside of the binary format, and is split off as an
// optimization (in J3D every material attribute is split off like this).
struct G3dShader {
  bool operator==(const G3dShader&) const = default;

  // Fixed-size DL
  librii::gx::SwapTable mSwapTable;
  rsl::array_vector<librii::gx::IndOrder, 4> mIndirectOrders;

  // Variable-sized DL
  rsl::array_vector<librii::gx::TevStage, 16> mStages;

  G3dShader(const librii::gx::LowLevelGxMaterial& mat)
      : mSwapTable(mat.mSwapTable) {
    mIndirectOrders.resize(mat.indirectStages.size());
    for (int i = 0; i < mIndirectOrders.size(); ++i)
      mIndirectOrders[i] = mat.indirectStages[i].order;

    mStages.resize(mat.mStages.size());
    for (int i = 0; i < mat.mStages.size(); ++i)
      mStages[i] = mat.mStages[i];
  }
};

struct G3dMaterialData : public librii::gx::GCMaterialData {
  rsl::array_vector<librii::g3d::G3dIndConfig, 4> indConfig;
  u32 flag = 0;
  u32 id; // Local
  s8 lightSetIndex = -1;
  s8 fogIndex = -1;

  bool operator==(const G3dMaterialData& rhs) const = default;
};

} // namespace librii::g3d
