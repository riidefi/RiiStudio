#pragma once

#include "GXMaterial.hpp"
#include <core/util/string_builder.hpp> // StringBuilder
#include <llvm/Support/Error.h>         // llvm::Error
#include <string>                       // std::string
#include <vendor/glm/vec4.hpp>

namespace libcube {

using StringBuilder = riistudio::util::StringBuilder;

enum class GfxFormat { F32_R, F32_RG, F32_RGB, F32_RGBA };

class GXProgram {
public:
  GXProgram(const GXMaterial& mat) : mMaterial(mat), mName(mat.name) {}
  ~GXProgram() = default;

protected:
  //----------------------------------
  // Annotations
  //----------------------------------
#define vec2_string std::string
#define vec3_string std::string
#define vec4_string std::string

  //----------------------------------
  // Lighting
  //----------------------------------
  llvm::Error generateMaterialSource(StringBuilder& builder,
                                     const gx::ChannelControl& chan, int i);
  llvm::Error generateAmbientSource(StringBuilder& builder,
                                    const gx::ChannelControl& chan, int i);
  llvm::Error generateLightDiffFn(StringBuilder& builder,
                                  const gx::ChannelControl& chan,
                                  const std::string& lightName);
  llvm::Error generateLightAttnFn(StringBuilder& builder,
                                  const gx::ChannelControl& chan,
                                  const std::string& lightName);
  llvm::Error generateColorChannel(StringBuilder& builder,
                                   const gx::ChannelControl& chan,
                                   const std::string& outputName, int i);
  llvm::Error generateLightChannel(StringBuilder& builder,
                                   const LightingChannelControl& lightChannel,
                                   const std::string& outputName, int i);
  llvm::Error generateLightChannels(StringBuilder& builder);

  //----------------------------------
  // Matrix
  //----------------------------------
  llvm::Error generateMulPntMatrixStatic(StringBuilder& builder,
                                         gx::PostTexMatrix pnt,
                                         const std::string& src);
  vec3_string generateMulPntMatrixDynamic(const std::string& attrStr,
                                          const vec4_string& src);
  std::string generateTexMtxIdxAttr(int index);

  //----------------------------------
  // Texture Coordinate Generation
  //----------------------------------
  vec4_string generateTexGenSource(gx::TexGenSrc src);
  vec3_string generatePostTexGenMatrixMult(const gx::TexCoordGen& texCoordGen,
                                           int id, const vec4_string& src);
  vec3_string generateTexGenMatrixMult(const gx::TexCoordGen& texCoordGen,
                                       int id, const vec3_string& src);
  vec3_string generateTexGenType(const gx::TexCoordGen& texCoordGen, int id,
                                 const vec4_string& src);
  vec3_string generateTexGenNrm(const gx::TexCoordGen& texCoordGen, int id);
  vec3_string generateTexGenPost(const gx::TexCoordGen& texCoordGen, int id);
  std::string generateTexGen(const gx::TexCoordGen& texCoordGen, int id);
  std::string generateTexGens();
  std::string generateTexCoordGetters();

  //----------------------------------
  // Indirect Texturing
  //----------------------------------
  std::string
  generateIndTexStageScaleN(gx::IndirectTextureScalePair::Selection scale);
  std::string
  generateIndTexStageScale(const gx::TevStage::IndirectStage& stage,
                           const gx::IndirectTextureScalePair& scale,
                           const gx::IndOrder& mIndOrder);

  std::string generateTextureSample(u32 index, const std::string& coord);
  std::string generateIndTexStage(u32 indTexStageIndex);
  std::string generateIndTexStages();

  //----------------------------------
  // TEV
  //----------------------------------
  // Constant Values
  std::string generateKonstColorSel(gx::TevKColorSel konstColor);
  std::string generateKonstAlphaSel(gx::TevKAlphaSel konstAlpha);

  // Fragment inputs
  std::string generateRas(const gx::TevStage& stage);
  std::string generateTexAccess(const gx::TevStage& stage);

  // Swizzling / Swap Table
  std::string generateComponentSwizzle(const gx::SwapTableEntry* swapTable,
                                       gx::ColorComponent channel);
  std::string generateColorSwizzle(const gx::SwapTableEntry* swapTable,
                                   gx::TevColorArg colorIn);

  // Color/Alpha Inputs
  std::string generateColorIn(const gx::TevStage& stage,
                              gx::TevColorArg colorIn);
  std::string generateAlphaIn(const gx::TevStage& stage,
                              gx::TevAlphaArg alphaIn);

  // Tev Misc
  std::string generateTevInputs(const gx::TevStage& stage);
  std::string generateTevRegister(gx::TevReg regId);
  std::string generateTevOpBiasScaleClamp(const std::string& value,
                                          gx::TevBias bias, gx::TevScale scale);

  // Tev Operands
  std::string generateTevOp(gx::TevColorOp op, gx::TevBias bias,
                            gx::TevScale scale, const std::string& a,
                            const std::string& b, const std::string& c,
                            const std::string& d, const std::string& zero);

  std::string generateTevOpValue(gx::TevColorOp op, gx::TevBias bias,
                                 gx::TevScale scale, bool clamp,
                                 const std::string& a, const std::string& b,
                                 const std::string& c, const std::string& d,
                                 const std::string& zero);

  std::string generateColorOp(const gx::TevStage& stage);
  std::string generateAlphaOp(const gx::TevStage& stage);

  // Indirect Texture Coordinate Transformation
  std::string generateTevTexCoordWrapN(const std::string& texCoord,
                                       gx::IndTexWrap wrap);
  std::string generateTevTexCoordWrap(const gx::TevStage& stage);
  std::string generateTevTexCoordIndTexCoordBias(const gx::TevStage& stage);
  std::string generateTevTexCoordIndTexCoord(const gx::TevStage& stage);
  std::string generateTevTexCoordIndirectMtx(const gx::TevStage& stage);
  std::string generateTevTexCoordIndirectTranslation(const gx::TevStage& stage);
  std::string generateTevTexCoordIndirect(const gx::TevStage& stage);
  std::string generateTevTexCoord(const gx::TevStage& stage);

  // Putting it all together
  llvm::Error generateTevStage(StringBuilder& builder, u32 tevStageIndex);
  llvm::Error generateTevStages(StringBuilder& builder);
  llvm::Error generateTevStagesLastMinuteFixup(StringBuilder& builder);

  //----------------------------------
  // Alpha Test
  //----------------------------------
  llvm::Error generateAlphaTestCompare(StringBuilder& builder,
                                       gx::Comparison compare, float reference);
  llvm::Error generateAlphaTestOp(StringBuilder& builder, gx::AlphaOp op);
  llvm::Error generateAlphaTest(StringBuilder& builder);

  //----------------------------------
  // Fog
  //----------------------------------
  std::string generateFogZCoord();
  std::string generateFogBase();
  std::string generateFogAdj(const std::string& base);
  std::string generateFogFunc(const std::string& base);
  std::string generateFog();

  //----------------------------------
  // Attributes
  //----------------------------------
  llvm::Error generateAttributeStorageType(StringBuilder& builder, u32 glFormat,
                                           u32 count);
  llvm::Error generateVertAttributeDefs(StringBuilder& builder);

  //----------------------------------
  // PNMTX
  //----------------------------------
  llvm::Error generateMulPos(StringBuilder& builder);
  llvm::Error generateMulNrm(StringBuilder& builder);

public:
  //----------------------------------
  // Tying it all up
  //----------------------------------

  //! @brief Generate a pair of GLSL shaders for the given GX material.
  //!
  //! @return vertex_source : fragment_source
  //!
  std::pair<std::string, std::string> generateShaders();

  GXMaterial mMaterial;
  std::string mName;
};
// Converts to GL
u32 translateCullMode(gx::CullMode cullMode);
u32 translateBlendFactorCommon(gx::BlendModeFactor factor);
u32 translateBlendSrcFactor(gx::BlendModeFactor factor);
u32 translateBlendDstFactor(gx::BlendModeFactor factor);
u32 translateCompareType(gx::Comparison compareType);

void translateGfxMegaState(MegaState& megaState, GXMaterial& material);
gx::ColorSelChanApi getRasColorChannelID(gx::ColorSelChanApi v);

} // namespace libcube
