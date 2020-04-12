#pragma once

#include "DLInterpreter.hpp"
#include "GPUMaterial.hpp"
#include <plugins/gc/GX/Struct/Shader.hpp>

#include <plugins/gc/export/Material.hpp>

#include <plugins/gc/export/IndexedPolygon.hpp>

#include <array>

namespace libcube::gpu {

class QDisplayListShaderHandler : public QDisplayListHandler {
public:
  QDisplayListShaderHandler(gx::Shader &shader, int numStages);
  ~QDisplayListShaderHandler();

  void onCommandBP(const QBPCommand &token) override;
  void onStreamEnd() override;

  gx::Shader &mShader;
  int mNumStages;

  struct {
    template <typename T> inline void setReg(T &reg, const QBPCommand &cmd) {
      reg.hex = (reg.hex & ~mMask) | (cmd.val & mMask);
    }

    u32 mMask = u32(-1); // BP mask. only valid for next

    std::array<gpu::TevKSel, 8> tevksel;
    gpu::RAS1_IREF iref;
    std::array<gpu::RAS1_TREF, 8> tref;

    std::array<ColorCombiner, 16> colorEnv;
    std::array<AlphaCombiner, 16> alphaEnv;
    std::array<TevStageIndirect, 16> indCmd;

    // Brawlbox bug makes this useful
    u16 definedIndCmds = 0;
    void defineIndCmd(int i) { definedIndCmds |= (1 << i); }
    bool isDefinedIndCmd(int i) { return definedIndCmds & (1 << i); }
  } mGpuShader;
};

struct GPUMaterial {
  // Display lists
  struct {
    std::vector<BPCommand> mMiscBPCommands;
    std::vector<CPCommand> mMiscCPCommands;
    std::vector<XFCommand> mMiscXFCommands;
  };
  struct PixelEngine {
    AlphaTest mAlphaCompare;
    ZMode mZMode;
    CMODE0 mBlendMode;
    CMODE1 mDstAlpha;
  } mPixel;
  struct {
    std::array<GPUTevReg, 4> Registers; // We never see 0 (TEVPREV) set
    std::array<GPUTevReg, 4> Konstants;
  } mShaderColor;
  struct {
    std::array<ras1_ss, 2> mIndTexScales;
    std::array<IND_MTX, 3> mIndMatrices;
  } mIndirect;
  std::array<XF_TEXTURE, 8> mTexture;

  u32 mMask = u32(-1); // BP mask. only valid for next command

  template <typename T> inline void setReg(T &reg, const QBPCommand &cmd) {
    reg.hex = (reg.hex & ~mMask) | (cmd.val & mMask);
  }

  GPUMaterial() {
    for (int i = 0; i < 8; i++) {
      mTexture[i].id = i;
    }
  }
};

class QDisplayListMaterialHandler : public QDisplayListHandler {
public:
  QDisplayListMaterialHandler(GCMaterialData &mat);
  ~QDisplayListMaterialHandler();

  void onCommandBP(const QBPCommand &token) override;
  void onCommandXF(const QXFCommand &token) override;
  void onStreamEnd() override;

  GCMaterialData &mMat;
  GPUMaterial mGpuMat;
};

class QDisplayListVertexSetupHandler : public QDisplayListHandler {
public:
  void onCommandBP(const QBPCommand &token) override;
  void onCommandCP(const QCPCommand &token) override;
  void onStreamEnd() override;

  GPUMesh mGpuMesh;
};

} // namespace libcube::gpu
