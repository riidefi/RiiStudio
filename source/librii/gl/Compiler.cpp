#include <glfw/glfw3.h>
#include <librii/gl/Compiler.hpp>
#include <rsl/StringBuilder.hpp>

IMPORT_STD;

namespace librii::gl {

struct LightingChannelControl {
  gx::ChannelControl colorChannel;
  gx::ChannelControl alphaChannel;
};

using StringBuilder = rsl::StringBuilder;

using namespace librii::gx;

const std::array<VertexAttributeGenDef, 16> vtxAttributeGenDefs{
    VertexAttributeGenDef{VertexAttribute::Position, "Position", GL_FLOAT, 3},
    VertexAttributeGenDef{VertexAttribute::PositionNormalMatrixIndex,
                          "PnMtxIdx", GL_FLOAT, 1},
    VertexAttributeGenDef{VertexAttribute::Texture0MatrixIndex, "TexMtx0123Idx",
                          GL_FLOAT, 4},
    VertexAttributeGenDef{VertexAttribute::Texture4MatrixIndex, "TexMtx4567Idx",
                          GL_FLOAT, 4},
    VertexAttributeGenDef{VertexAttribute::Normal, "Normal", GL_FLOAT, 3},
    VertexAttributeGenDef{VertexAttribute::Color0, "Color0", GL_FLOAT, 4},
    VertexAttributeGenDef{VertexAttribute::Color1, "Color1", GL_FLOAT, 4},
    VertexAttributeGenDef{VertexAttribute::TexCoord0, "Tex0", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::TexCoord1, "Tex1", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::TexCoord2, "Tex2", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::TexCoord3, "Tex3", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::TexCoord4, "Tex4", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::TexCoord5, "Tex5", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::TexCoord6, "Tex6", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::TexCoord7, "Tex7", GL_FLOAT, 2},
    VertexAttributeGenDef{VertexAttribute::Max, "PrimID", GL_FLOAT, 4},
};

Result<std::pair<const VertexAttributeGenDef*, std::size_t>>
getVertexAttribGenDef(VertexAttribute vtxAttrib) {
  if (vtxAttrib == VertexAttribute::Texture1MatrixIndex ||
      vtxAttrib == VertexAttribute::Texture2MatrixIndex ||
      vtxAttrib == VertexAttribute::Texture3MatrixIndex)
    vtxAttrib = VertexAttribute::Texture0MatrixIndex;

  const auto it = std::ranges::find_if(
      vtxAttributeGenDefs, [vtxAttrib](const VertexAttributeGenDef& def) {
        return def.attrib == vtxAttrib;
      });

  EXPECT(it != vtxAttributeGenDefs.end());

  return std::pair{&*it, it - vtxAttributeGenDefs.begin()};
}

std::string generateBindingsDefinition(bool postTexMtxBlock, bool lightsBlock) {
#if defined(__EMSCRIPTEN__) || defined(__APPLE__)
  return std::string(R"(
// Expected to be constant across the entire scene.
layout(std140) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

#define u_SceneTextureLODBias u_Misc0[0]
struct Light {
    vec4 Color;
    vec4 Position;
    vec4 Direction;
    vec4 DistAtten;
    vec4 CosAtten;
};
// Expected to change with each material.
layout(std140, row_major) uniform ub_MaterialParams {
    vec4 u_ColorMatReg[2];
    vec4 u_ColorAmbReg[2];
    vec4 u_KonstColor[4];
    vec4 u_Color[4];
    mat4x3 u_TexMtx[10]; //4x3
    // SizeX, SizeY, 0, Bias
    vec4 u_TextureParams[8];
    mat4x2 u_IndTexMtx[3]; // 4x2
    // Optional parameters.)") +
         "\n" + (postTexMtxBlock ? "Mat4x3 u_PostTexMtx[20];\n" : "") + // 4x3
         (lightsBlock ? "Light u_LightParams[8];\n" : "") +
         std::string("};\n") +
         "// Expected to change with each shape packet.\n"
         "layout(std140, row_major) uniform ub_PacketParams {\n"
         "    mat4x3 u_PosMtx[10];\n" // 4x3
         "};\n"
         "uniform sampler2D u_Texture[8];\n";
#else
  return std::string(R"(
// Expected to be constant across the entire scene.
layout(std140, binding=0) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

#define u_SceneTextureLODBias u_Misc0[0]
struct Light {
    vec4 Color;
    vec4 Position;
    vec4 Direction;
    vec4 DistAtten;
    vec4 CosAtten;
};
// Expected to change with each material.
layout(std140, row_major, binding=1) uniform ub_MaterialParams {
    vec4 u_ColorMatReg[2];
    vec4 u_ColorAmbReg[2];
    vec4 u_KonstColor[4];
    vec4 u_Color[4];
    mat4x3 u_TexMtx[10]; //4x3
    // SizeX, SizeY, 0, Bias
    vec4 u_TextureParams[8];
    mat4x2 u_IndTexMtx[3]; // 4x2
    // Optional parameters.)") +
         "\n" + (postTexMtxBlock ? "Mat4x3 u_PostTexMtx[20];\n" : "") + // 4x3
         (lightsBlock ? "Light u_LightParams[8];\n" : "") +
         std::string("};\n") +
         "// Expected to change with each shape packet.\n"
         "layout(std140, row_major, binding=2) uniform ub_PacketParams {\n"
         "    mat4x3 u_PosMtx[10];\n" // 4x3
         "};\n"
         "uniform sampler2D u_Texture[8];\n";
#endif
}

class GXProgram {
public:
  GXProgram(const gx::LowLevelGxMaterial& mat, std::string_view name)
      : mMaterial(mat), mName(name) {}
  ~GXProgram() = default;

  Result<void> generateMaterialSource(StringBuilder& builder,
                                      const gx::ChannelControl& chan, int i) {
    switch (chan.Material) {
    case ColorSource::Vertex:
      builder += "a_Color";
      builder += std::to_string(i);
      break;
    case ColorSource::Register:
      builder += "u_ColorMatReg[";
      builder += std::to_string(i);
      builder += "]";
      break;
    }
    return {};
  }

  Result<void> generateAmbientSource(StringBuilder& builder,
                                     const gx::ChannelControl& chan, int i) {
    switch (chan.Ambient) {
    case ColorSource::Vertex:
      builder += "a_Color";
      builder += std::to_string(i);
      break;
    case ColorSource::Register:
      builder += "u_ColorAmbReg[";
      builder += std::to_string(i);
      builder += "]";
      break;
    }
    return {};
  }

  Result<void> generateLightDiffFn(StringBuilder& builder,
                                   const gx::ChannelControl& chan,
                                   const std::string& lightName) {
    const char* NdotL = "dot(t_Normal, t_LightDeltaDir)";

    switch (chan.diffuseFn) {
    default:
    case DiffuseFunction::None:
      builder += "1.0";
      break;
    case DiffuseFunction::Sign:
      builder += NdotL;
      break;
    case DiffuseFunction::Clamp:
      builder += "max(";
      builder += NdotL;
      builder += ", 0.0f)";
      break;
    }
    return {};
  }
  Result<void> generateLightAttnFn(StringBuilder& builder,
                                   const gx::ChannelControl& chan,
                                   const std::string& lightName) {
    if (chan.attenuationFn == AttenuationFunction::None) {
      builder += "t_Attenuation = 1.0;";
    } else if (chan.attenuationFn == AttenuationFunction::Spotlight) {
      auto attn = std::string("max(0.0, dot(t_LightDeltaDir, ") + lightName +
                  ".Direction.xyz))";
      auto cosAttn = std::string("max(0.0, ApplyAttenuation(") + lightName +
                     ".CosAtten.xyz, " + attn + "))";
      auto distAttn =
          std::string("dot(") + lightName +
          ".DistAtten.xyz, vec3(1.0, t_LightDeltaDist, t_LightDeltaDist2))";

      builder += "t_Attenuation = ";
      builder += cosAttn;
      builder += " / ";
      builder += distAttn;
      builder += ";";
    } else if (chan.attenuationFn == AttenuationFunction::Specular) {
      auto attn = std::string("(dot(t_Normal, t_LightDeltaDir) >= 0.0) ? "
                              "max(0.0, dot(t_Normal, ") +
                  lightName + ".Direction.xyz)) : 0.0";
      auto cosAttn = std::string("ApplyAttenuation(") + lightName +
                     ".CosAtten.xyz, t_Attenuation)";
      auto distAttn = std::string("ApplyAttenuation(") + lightName +
                      ".DistAtten.xyz, t_Attenuation)";

      builder += "t_Attenuation = ";
      builder += attn;
      builder += ";\n";

      builder += "t_Attenuation = ";
      builder += cosAttn;
      builder += " / ";
      builder += distAttn;
      builder += ";";
    } else {
      return std::unexpected("Invalid attenuation function");
    }

    return {};
  }
  Result<void> generateColorChannel(StringBuilder& builder,
                                    const gx::ChannelControl& chan,
                                    const std::string& outputName, int i) {

    if (chan.enabled) {
      builder += "t_LightAccum = ";
      TRY(generateAmbientSource(builder, chan, i));
      builder += ";\n";

      if (chan.lightMask != LightID::None) {
        EXPECT(hasLightsBlock, "Channel has a non-empty lightmask yet the "
                               "shader does not contain the lights block");
      }

      for (int j = 0; j < 8; j++) {
        if (!(u32(chan.lightMask) & (1 << j)))
          continue;

        const std::string lightName =
            "u_LightParams[" + std::to_string(j) + "]";

        builder += "    t_LightDelta = ";
        builder += lightName;
        builder += ".Position.xyz - v_Position.xyz;\n"
                   "    t_LightDeltaDist2 = dot(t_LightDelta, t_LightDelta);\n"
                   "    t_LightDeltaDist = sqrt(t_LightDeltaDist2);\n"
                   "    t_LightDeltaDir = t_LightDelta / t_LightDeltaDist;\n";

        TRY(generateLightAttnFn(builder, chan, lightName));
        builder += "    t_LightAccum += ";
        TRY(generateLightDiffFn(builder, chan, lightName));
        builder += " * t_Attenuation * " + lightName + ".Color;\n";
      }
    } else {
      // Without lighting, everything is full-bright.
      builder += "    t_LightAccum = vec4(1.0);\n";
    }

    builder += "    ";
    builder += outputName;
    builder += " = ";
    TRY(generateMaterialSource(builder, chan, i));
    builder += " * clamp(t_LightAccum, 0.0, 1.0);\n";

    return {};
  }

  Result<void> generateLightChannel(StringBuilder& builder,
                                    const LightingChannelControl& lightChannel,
                                    const std::string& outputName, int i) {
    if (lightChannel.colorChannel == lightChannel.alphaChannel) {
      // TODO
      builder += "    ";
      TRY(generateColorChannel(builder, lightChannel.colorChannel, outputName,
                               i));
    } else {
      TRY(generateColorChannel(builder, lightChannel.colorChannel,
                               "t_ColorChanTemp", i));
      builder += "\n" + outputName + ".rgb = t_ColorChanTemp.rgb;\n";
      TRY(generateColorChannel(builder, lightChannel.alphaChannel,
                               "t_ColorChanTemp", i));
      builder += "\n" + outputName + ".a = t_ColorChanTemp.a;\n";
    }

    return {};
  }

  Result<void> generateLightChannels(StringBuilder& builder) {
    const auto& src = mMaterial.colorChanControls;

    std::array<LightingChannelControl, 2> ctrl;

    if (src.size() > 0)
      ctrl[0].colorChannel = src[0];
    if (src.size() > 1)
      ctrl[0].alphaChannel = src[1];
    if (src.size() > 2)
      ctrl[1].colorChannel = src[2];
    if (src.size() > 3)
      ctrl[1].alphaChannel = src[3];

    int i = 0;
    for (const auto& chan : ctrl) {
      TRY(generateLightChannel(builder, chan, "v_Color" + std::to_string(i),
                               i));
      builder += "\n";
      ++i;
    }

    return {};
  }

  // Matrix
  Result<void> generateMulPntMatrixStatic(StringBuilder& builder,
                                          gx::PostTexMatrix pnt,
                                          const std::string& src) {
    // TODO
    if (pnt == gx::PostTexMatrix::Identity ||
        (int)pnt == (int)gx::TexMatrix::Identity) {
      builder += src;
      builder += ".xyz";
      return {};
    }

    if (pnt >= gx::PostTexMatrix::Matrix0) {
      const int pnMtxIdx = (((int)pnt - (int)gx::PostTexMatrix::Matrix0)) / 3;
      builder += "(u_PosMtx[";
      builder += std::to_string(pnMtxIdx);
      builder += "] * ";
      builder += src;
      builder += ")";
      return {};
    }

    if ((int)pnt >= (int)gx::TexMatrix::TexMatrix0) {
      const int texMtxIdx = (((int)pnt - (int)gx::TexMatrix::TexMatrix0)) / 3;
      builder += "(u_TexMtx[";
      builder += std::to_string(texMtxIdx);
      builder += "] * ";
      builder += src;
      builder += ")";
      return {};
    }

    return std::unexpected("Invalid posttexmatrix");
  }
  // Output is a vec3, src is a vec4.
  std::string generateMulPntMatrixDynamic(const std::string& attrStr,
                                          const std::string& src) {
    return "(GetPosTexMatrix(" + attrStr + ") * " + src + ")";
  }
  std::string generateTexMtxIdxAttr(int index) {
    switch (index) {
    case 0:
      return "a_TexMtx0123Idx.x";
    case 1:
      return "a_TexMtx0123Idx.y";
    case 2:
      return "a_TexMtx0123Idx.z";
    case 3:
      return "a_TexMtx0123Idx.w";
    case 4:
      return "a_TexMtx4567Idx.x";
    case 5:
      return "a_TexMtx4567Idx.y";
    case 6:
      return "a_TexMtx4567Idx.z";
    case 7:
      return "a_TexMtx4567Idx.w";
    default:
      return "INVALID";
    }

    return "";
  }
  //----------------------------------
  // TexGen

  // Output is a vec4.
  // #if 0
  std::string generateTexGenSource(gx::TexGenSrc src) {
    switch (src) {
    case gx::TexGenSrc::Position:
      return "vec4(a_Position, 1.0)";
    case gx::TexGenSrc::Normal:
      return "vec4(a_Normal, 1.0)";
    case gx::TexGenSrc::Color0:
      return "v_Color0";
    case gx::TexGenSrc::Color1:
      return "v_Color1";
    case gx::TexGenSrc::UV0:
      return "vec4(a_Tex0, 1.0, 1.0)";
    case gx::TexGenSrc::UV1:
      return "vec4(a_Tex1, 1.0, 1.0)";
    case gx::TexGenSrc::UV2:
      return "vec4(a_Tex2, 1.0, 1.0)";
    case gx::TexGenSrc::UV3:
      return "vec4(a_Tex3, 1.0, 1.0)";
    case gx::TexGenSrc::UV4:
      return "vec4(a_Tex4, 1.0, 1.0)";
    case gx::TexGenSrc::UV5:
      return "vec4(a_Tex5, 1.0, 1.0)";
    case gx::TexGenSrc::UV6:
      return "vec4(a_Tex6, 1.0, 1.0)";
    case gx::TexGenSrc::UV7:
      return "vec4(a_Tex7, 1.0, 1.0)";
    case gx::TexGenSrc::BumpUV0:
      return "vec4(v_TexCoord0, 1.0)";
    case gx::TexGenSrc::BumpUV1:
      return "vec4(v_TexCoord1, 1.0)";
    case gx::TexGenSrc::BumpUV2:
      return "vec4(v_TexCoord2, 1.0)";
    case gx::TexGenSrc::BumpUV3:
      return "vec4(v_TexCoord3, 1.0)";
    case gx::TexGenSrc::BumpUV4:
      return "vec4(v_TexCoord4, 1.0)";
    case gx::TexGenSrc::BumpUV5:
      return "vec4(v_TexCoord5, 1.0)";
    case gx::TexGenSrc::BumpUV6:
      return "vec4(v_TexCoord6, 1.0)";
    default:
      return "INVALID";
    }
    return "";
  }
  // Output is a vec3, src is a vec4.
  std::string generatePostTexGenMatrixMult(const gx::TexCoordGen& texCoordGen,
                                           int id, const std::string& src) {
    // TODO:
    if (true || texCoordGen.postMatrix == gx::PostTexMatrix::Identity) {
      return src + ".xyz";
    } else if (texCoordGen.postMatrix >= gx::PostTexMatrix::Matrix0) {
      const u32 texMtxIdx =
          ((u32)texCoordGen.postMatrix - (u32)gx::PostTexMatrix::Matrix0) / 3;
      assert(texMtxIdx < 10);
      return "(u_PostTexMtx[" + std::to_string(texMtxIdx) +
             std::string("] * ") + src + ")";
    } else {
      return "INVALID";
      return "";
    }
  }
  // Output is a vec3, src is a vec3.
  std::expected<std::string, std::string>
  generateTexGenMatrixMult(const gx::TexCoordGen& texCoordGen, int id,
                           const std::string& src) {
    // TODO: Will ID ever be different from index?

    // Dynamic TexMtxIdx is off by default.
    if (useTexMtxIdx[id]) {
      const std::string attrStr = generateTexMtxIdxAttr(id);
      return generateMulPntMatrixDynamic(attrStr, src);
    } else {
      // TODO: Verify
      std::array<char, 256> buf{};
      StringBuilder builder(buf.data(), buf.size());
      TRY(generateMulPntMatrixStatic(
          builder, static_cast<librii::gx::PostTexMatrix>(texCoordGen.matrix),
          src));
      return buf.data();
    }
  }

  // Output is a vec3, src is a vec4.
  std::expected<std::string, std::string>
  generateTexGenType(const gx::TexCoordGen& texCoordGen, int id,
                     const std::string& src) {
    switch (texCoordGen.func) {
    case gx::TexGenType::SRTG:
      return "vec3(" + src + ".xy, 1.0)";
    case gx::TexGenType::Matrix2x4:
      return std::format("vec3({}.xy, 1.0)",
                         TRY(generateTexGenMatrixMult(texCoordGen, id, src)));
    case gx::TexGenType::Matrix3x4:
      return TRY(generateTexGenMatrixMult(texCoordGen, id, src));
    case gx::TexGenType::Bump0:
    case gx::TexGenType::Bump1:
    case gx::TexGenType::Bump2:
    case gx::TexGenType::Bump3:
    case gx::TexGenType::Bump4:
    case gx::TexGenType::Bump5:
    case gx::TexGenType::Bump6:
    case gx::TexGenType::Bump7:
      return "vec3(0.5, 0.5, 0.5)";
    default:
      return std::unexpected(std::format("Invalid TexGenType: {}",
                                         static_cast<u32>(texCoordGen.func)));
    }
  }

  // Output is a vec3.
  std::expected<std::string, std::string>
  generateTexGenNrm(const gx::TexCoordGen& texCoordGen, int id) {
    const auto src = generateTexGenSource(texCoordGen.sourceParam);
    const auto type = TRY(generateTexGenType(texCoordGen, id, src));
    if (texCoordGen.normalize)
      return "normalize(" + type + ")";
    else
      return type;
  }
  // Output is a vec3.
  std::expected<std::string, std::string>
  generateTexGenPost(const gx::TexCoordGen& texCoordGen, int id) {
    const auto src = TRY(generateTexGenNrm(texCoordGen, id));
    // TODO
    if (true || texCoordGen.postMatrix == gx::PostTexMatrix::Identity)
      return src;
    else
      return generatePostTexGenMatrixMult(texCoordGen, id,
                                          "vec4(" + src + ", 1.0)");
  }

  std::expected<std::string, std::string>
  generateTexGen(const gx::TexCoordGen& texCoordGen, int id) {
    return "v_TexCoord" + std::to_string(/*texCoordGen.*/ id) + " = " +
           TRY(generateTexGenPost(texCoordGen, id)) + ";\n";
  }

  std::expected<std::string, std::string> generateTexGens() {
    std::string out;
    const auto& tgs = mMaterial.texGens;
    for (int i = 0; i < tgs.size(); ++i)
      out += TRY(generateTexGen(tgs[i], i));
    return out;
  }

  std::string generateTexCoordGetters() {
    std::string out;
    for (int i = 0; i < mMaterial.texGens.size(); ++i) {
      const std::string is = std::to_string(i);
      out += "vec2 ReadTexCoord" + is + "() { return v_TexCoord" + is +
             ".xy / v_TexCoord" + is + ".z; }\n";
    }
    return out;
  }

  // IndTex
  Result<std::string>
  generateIndTexStageScaleN(gx::IndirectTextureScalePair::Selection scale) {
    switch (scale) {
    case gx::IndirectTextureScalePair::Selection::x_1:
      return "1.0";
    case gx::IndirectTextureScalePair::Selection::x_2:
      return "1.0/2.0";
    case gx::IndirectTextureScalePair::Selection::x_4:
      return "1.0/4.0";
    case gx::IndirectTextureScalePair::Selection::x_8:
      return "1.0/8.0";
    case gx::IndirectTextureScalePair::Selection::x_16:
      return "1.0/16.0";
    case gx::IndirectTextureScalePair::Selection::x_32:
      return "1.0/32.0";
    case gx::IndirectTextureScalePair::Selection::x_64:
      return "1.0/64.0";
    case gx::IndirectTextureScalePair::Selection::x_128:
      return "1.0/128.0";
    case gx::IndirectTextureScalePair::Selection::x_256:
      return "1.0/256.0";
    }
    EXPECT(false, "Invalid IndTexScale");
  }

  Result<std::string>
  generateIndTexStageScale(const gx::TevStage::IndirectStage& stage,
                           const gx::IndirectTextureScalePair& scale,
                           const gx::IndOrder& mIndOrder) {
    const std::string baseCoord =
        "ReadTexCoord" + std::to_string(mIndOrder.refCoord) + "()";

    if (scale.U == gx::IndirectTextureScalePair::Selection::x_1 &&
        scale.V == gx::IndirectTextureScalePair::Selection::x_1)
      return baseCoord;
    else
      return std::format("{} * vec2({}, {})", baseCoord,
                         TRY(generateIndTexStageScaleN(scale.U)),
                         TRY(generateIndTexStageScaleN(scale.V)));
  }

  std::string generateTextureSample(u32 index, const std::string& coord) {
    const auto idx_str = std::to_string(index);
    return "texture(u_Texture[" + idx_str + "], " + coord +
           ", TextureLODBias(" + idx_str + "))";
  }

  [[nodiscard]] Result<void> generateIndTexStage(std::string& out,
                                                 u32 indTexStageIndex) {
    const auto& stage = mMaterial.mStages[indTexStageIndex].indirectStage;

    const auto scale = indTexStageIndex >= mMaterial.indirectStages.size()
                           ? IndirectTextureScalePair{}
                           : mMaterial.indirectStages[indTexStageIndex].scale;
    const auto order = indTexStageIndex >= mMaterial.indirectStages.size()
                           ? IndOrder{}
                           : mMaterial.indirectStages[indTexStageIndex].order;

    out += "vec3 t_IndTexCoord";
    out += std::to_string(indTexStageIndex);
    out += " = ";

    out += "255.0 * ";
    out += generateTextureSample(
        order.refMap, TRY(generateIndTexStageScale(stage, scale, order)));
    out += ".abg;\n";
    return {};
  }

  Result<std::string> generateIndTexStages() {
    std::string out;
    auto& matData = mMaterial;

    for (std::size_t i = 0; i < matData.indirectStages.size(); ++i) {
      if (matData.indirectStages[i].order.refCoord >= matData.texGens.size())
        continue;
      // if (matData.indirectStages[i].order.refMap >= matData.samplers.size())
      //   continue;

      TRY(generateIndTexStage(out, i));
    }
    return out;
  }

  // TEV
  Result<std::string> generateKonstColorSel(gx::TevKColorSel konstColor) {
    switch (konstColor) {
    case gx::TevKColorSel::const_8_8:
      return "vec3(8.0/8.0)";
    case gx::TevKColorSel::const_7_8:
      return "vec3(7.0/8.0)";
    case gx::TevKColorSel::const_6_8:
      return "vec3(6.0/8.0)";
    case gx::TevKColorSel::const_5_8:
      return "vec3(5.0/8.0)";
    case gx::TevKColorSel::const_4_8:
      return "vec3(4.0/8.0)";
    case gx::TevKColorSel::const_3_8:
      return "vec3(3.0/8.0)";
    case gx::TevKColorSel::const_2_8:
      return "vec3(2.0/8.0)";
    case gx::TevKColorSel::const_1_8:
      return "vec3(1.0/8.0)";
    case gx::TevKColorSel::k0:
      return "s_kColor0.rgb";
    case gx::TevKColorSel::k0_r:
      return "s_kColor0.rrr";
    case gx::TevKColorSel::k0_g:
      return "s_kColor0.ggg";
    case gx::TevKColorSel::k0_b:
      return "s_kColor0.bbb";
    case gx::TevKColorSel::k0_a:
      return "s_kColor0.aaa";
    case gx::TevKColorSel::k1:
      return "s_kColor1.rgb";
    case gx::TevKColorSel::k1_r:
      return "s_kColor1.rrr";
    case gx::TevKColorSel::k1_g:
      return "s_kColor1.ggg";
    case gx::TevKColorSel::k1_b:
      return "s_kColor1.bbb";
    case gx::TevKColorSel::k1_a:
      return "s_kColor1.aaa";
    case gx::TevKColorSel::k2:
      return "s_kColor2.rgb";
    case gx::TevKColorSel::k2_r:
      return "s_kColor2.rrr";
    case gx::TevKColorSel::k2_g:
      return "s_kColor2.ggg";
    case gx::TevKColorSel::k2_b:
      return "s_kColor2.bbb";
    case gx::TevKColorSel::k2_a:
      return "s_kColor2.aaa";
    case gx::TevKColorSel::k3:
      return "s_kColor3.rgb";
    case gx::TevKColorSel::k3_r:
      return "s_kColor3.rrr";
    case gx::TevKColorSel::k3_g:
      return "s_kColor3.ggg";
    case gx::TevKColorSel::k3_b:
      return "s_kColor3.bbb";
    case gx::TevKColorSel::k3_a:
      return "s_kColor3.aaa";
    }
    EXPECT(false, "Invalid TevKColorSel");
  }

  Result<std::string> generateKonstAlphaSel(gx::TevKAlphaSel konstAlpha) {
    switch (konstAlpha) {
    default: // k0/k1/k2/k3 not valid for alpha
    case gx::TevKAlphaSel::const_8_8:
      return "(8.0/8.0)";
    case gx::TevKAlphaSel::const_7_8:
      return "(7.0/8.0)";
    case gx::TevKAlphaSel::const_6_8:
      return "(6.0/8.0)";
    case gx::TevKAlphaSel::const_5_8:
      return "(5.0/8.0)";
    case gx::TevKAlphaSel::const_4_8:
      return "(4.0/8.0)";
    case gx::TevKAlphaSel::const_3_8:
      return "(3.0/8.0)";
    case gx::TevKAlphaSel::const_2_8:
      return "(2.0/8.0)";
    case gx::TevKAlphaSel::const_1_8:
      return "(1.0/8.0)";
    case gx::TevKAlphaSel::k0_r:
      return "s_kColor0.r";
    case gx::TevKAlphaSel::k0_g:
      return "s_kColor0.g";
    case gx::TevKAlphaSel::k0_b:
      return "s_kColor0.b";
    case gx::TevKAlphaSel::k0_a:
      return "s_kColor0.a";
    case gx::TevKAlphaSel::k1_r:
      return "s_kColor1.r";
    case gx::TevKAlphaSel::k1_g:
      return "s_kColor1.g";
    case gx::TevKAlphaSel::k1_b:
      return "s_kColor1.b";
    case gx::TevKAlphaSel::k1_a:
      return "s_kColor1.a";
    case gx::TevKAlphaSel::k2_r:
      return "s_kColor2.r";
    case gx::TevKAlphaSel::k2_g:
      return "s_kColor2.g";
    case gx::TevKAlphaSel::k2_b:
      return "s_kColor2.b";
    case gx::TevKAlphaSel::k2_a:
      return "s_kColor2.a";
    case gx::TevKAlphaSel::k3_r:
      return "s_kColor3.r";
    case gx::TevKAlphaSel::k3_g:
      return "s_kColor3.g";
    case gx::TevKAlphaSel::k3_b:
      return "s_kColor3.b";
    case gx::TevKAlphaSel::k3_a:
      return "s_kColor3.a";
    }
    EXPECT(false, "Invalid TevKAlphaSel");
  }

  std::expected<std::string, std::string>
  generateRas(const gx::TevStage& stage) {
    switch (stage.rasOrder) {
    case gx::ColorSelChanApi::color0: // For custom files..
    case gx::ColorSelChanApi::alpha0:
    case gx::ColorSelChanApi::color0a0: // Real files will only use this
      return "v_Color0";
    case gx::ColorSelChanApi::color1: // For custom files..
    case gx::ColorSelChanApi::alpha1:
    case gx::ColorSelChanApi::color1a1: // Real files will only use this
      return "v_Color1";
    case gx::ColorSelChanApi::zero:
    case gx::ColorSelChanApi::null:
      return "vec4(0, 0, 0, 0)";
    case gx::ColorSelChanApi::ind_alpha:
      return std::unexpected("ind_alpha is not supported");
    case gx::ColorSelChanApi::normalized_ind_alpha:
      return std::unexpected("normalized_ind_alpha is not supported");
    }
    return std::unexpected(std::format("Invalid TEV rasOrder: {}",
                                       static_cast<u32>(stage.rasOrder)));
  }

  std::string generateTexAccess(const gx::TevStage& stage) {
    if (stage.texMap == 0xff)
      return "vec4(1.0, 1.0, 1.0, 1.0)";

    return generateTextureSample(stage.texMap, "t_TexCoord");
  }
  std::string generateComponentSwizzle(const gx::SwapTableEntry* swapTable,
                                       gx::ColorComponent channel) {
    const char* suffixes[] = {"r", "g", "b", "a"};
    if (swapTable)
      channel = swapTable->lookup(channel);
    // For sunshine common.szs\halfwhiteball.bmd
    if ((u8)channel >= 4)
      return "a";
    return suffixes[(u8)channel];
  }

  std::string generateColorSwizzle(const gx::SwapTableEntry* swapTable,
                                   gx::TevColorArg colorIn) {
    const auto swapR =
        generateComponentSwizzle(swapTable, gx::ColorComponent::r);
    const auto swapG =
        generateComponentSwizzle(swapTable, gx::ColorComponent::g);
    const auto swapB =
        generateComponentSwizzle(swapTable, gx::ColorComponent::b);
    const auto swapA =
        generateComponentSwizzle(swapTable, gx::ColorComponent::a);

    switch (colorIn) {
    case gx::TevColorArg::texc:
    case gx::TevColorArg::rasc:
      return swapR + swapG + swapB;
    case gx::TevColorArg::texa:
    case gx::TevColorArg::rasa:
      return swapA + swapA + swapA;
    default:
      return "INVALID";
    }
  }

  std::expected<std::string, std::string>
  generateColorIn(const gx::TevStage& stage, gx::TevColorArg colorIn) {
    switch (colorIn) {
    case gx::TevColorArg::cprev:
      return "t_ColorPrev.rgb";
    case gx::TevColorArg::aprev:
      return "t_ColorPrev.aaa";
    case gx::TevColorArg::c0:
      return "t_Color0.rgb";
    case gx::TevColorArg::a0:
      return "t_Color0.aaa";
    case gx::TevColorArg::c1:
      return "t_Color1.rgb";
    case gx::TevColorArg::a1:
      return "t_Color1.aaa";
    case gx::TevColorArg::c2:
      return "t_Color2.rgb";
    case gx::TevColorArg::a2:
      return "t_Color2.aaa";
    case gx::TevColorArg::texc:
      return generateTexAccess(stage) + "." +
             generateColorSwizzle(&mMaterial.mSwapTable[stage.texMapSwap],
                                  colorIn);
    case gx::TevColorArg::texa:
      return generateTexAccess(stage) + "." +
             generateColorSwizzle(&mMaterial.mSwapTable[stage.texMapSwap],
                                  colorIn);
    case gx::TevColorArg::rasc:
      return std::format(
          "TevSaturate({}.{})", TRY(generateRas(stage)),
          generateColorSwizzle(&mMaterial.mSwapTable[stage.rasSwap], colorIn));
    case gx::TevColorArg::rasa:
      return std::format(
          "TevSaturate({}.{})", TRY(generateRas(stage)),
          generateColorSwizzle(&mMaterial.mSwapTable[stage.rasSwap], colorIn));
    case gx::TevColorArg::one:
      return "vec3(1)";
    case gx::TevColorArg::half:
      return "vec3(1.0/2.0)";
    case gx::TevColorArg::konst:
      return generateKonstColorSel(stage.colorStage.constantSelection);
    case gx::TevColorArg::zero:
      return "vec3(0)";
    }
    EXPECT(false, "Invalid TevColorArg");
  }

  std::expected<std::string, std::string>
  generateAlphaIn(const gx::TevStage& stage, gx::TevAlphaArg alphaIn) {
    switch (alphaIn) {
    case gx::TevAlphaArg::aprev:
      return "t_ColorPrev.a";
    case gx::TevAlphaArg::a0:
      return "t_Color0.a";
    case gx::TevAlphaArg::a1:
      return "t_Color1.a";
    case gx::TevAlphaArg::a2:
      return "t_Color2.a";
    case gx::TevAlphaArg::texa:
      EXPECT(stage.texMapSwap < mMaterial.mSwapTable.size());
      return std::format(
          "{}.{}", generateTexAccess(stage),
          generateComponentSwizzle(&mMaterial.mSwapTable[stage.texMapSwap],
                                   gx::ColorComponent::a));
    case gx::TevAlphaArg::rasa:
      EXPECT(stage.rasSwap < mMaterial.mSwapTable.size());
      return std::format(
          "TevSaturate({}.{})", TRY(generateRas(stage)),
          generateComponentSwizzle(&mMaterial.mSwapTable[stage.rasSwap],
                                   gx::ColorComponent::a));
    case gx::TevAlphaArg::konst:
      return generateKonstAlphaSel(stage.alphaStage.constantSelection);
    case gx::TevAlphaArg::zero:
      return "0.0";
    }
    EXPECT(false, "Invalid TevAlphaArg");
  }

  std::expected<std::string, std::string>
  generateTevInputs(const gx::TevStage& stage) {
    return R"(
    t_TevA = TevOverflow(vec4()"
           "\n        " +
           TRY(generateColorIn(stage, stage.colorStage.a)) + ",\n        " +
           TRY(generateAlphaIn(stage, stage.alphaStage.a)) +
           "\n    "
           R"());
    t_TevB = TevOverflow(vec4()"
           "\n        " +
           TRY(generateColorIn(stage, stage.colorStage.b)) + ",\n        " +
           TRY(generateAlphaIn(stage, stage.alphaStage.b)) +
           "\n    "
           R"());
    t_TevC = TevOverflow(vec4()"
           "\n        " +
           TRY(generateColorIn(stage, stage.colorStage.c)) + ",\n        " +
           TRY(generateAlphaIn(stage, stage.alphaStage.c)) +
           "\n    "
           R"());
    t_TevD = vec4()"
           "\n        " +
           TRY(generateColorIn(stage, stage.colorStage.d)) + ",\n        " +
           TRY(generateAlphaIn(stage, stage.alphaStage.d)) +
           "\n    );\n"; //.trim();
  }

  std::expected<std::string, std::string>
  generateTevRegister(gx::TevReg regId) {
    switch (regId) {
    case gx::TevReg::prev:
      return "t_ColorPrev";
    case gx::TevReg::reg0:
      return "t_Color0";
    case gx::TevReg::reg1:
      return "t_Color1";
    case gx::TevReg::reg2:
      return "t_Color2";
    }
    return std::unexpected(
        std::format("Invalid TEV register id {}", static_cast<u32>(regId)));
  }

  std::string generateTevOpBiasScaleClamp(const std::string& value,
                                          gx::TevBias bias,
                                          gx::TevScale scale) {
    auto v = value;

    if (bias == gx::TevBias::add_half)
      v = "TevBias(" + v + ", 0.5)";
    else if (bias == gx::TevBias::sub_half)
      v = "TevBias(" + v + ", -0.5)";

    if (scale == gx::TevScale::scale_2)
      v = "(" + v + ") * 2.0";
    else if (scale == gx::TevScale::scale_4)
      v = "(" + v + ") * 4.0";
    else if (scale == gx::TevScale::divide_2)
      v = "(" + v + ") * 0.5";

    return v;
  }

  std::expected<std::string, std::string>
  generateTevOp(gx::TevColorOp op, gx::TevBias bias, gx::TevScale scale,
                const std::string& a, const std::string& b,
                const std::string& c, const std::string& d,
                const std::string& zero) {
    switch (op) {
    case gx::TevColorOp::add:
    case gx::TevColorOp::subtract: {
      const auto neg = (op == gx::TevColorOp::subtract) ? "-" : "";
      const auto v =
          neg + std::string("mix(") + a + ", " + b + ", " + c + ") + " + d;
      return generateTevOpBiasScaleClamp(v, bias, scale);
    }
    case gx::TevColorOp::comp_r8_gt:
      return "((t_TevA.r >  t_TevB.r) ? " + c + " : " + zero + ") + " + d;
    case gx::TevColorOp::comp_r8_eq:
      return "((t_TevA.r == t_TevB.r) ? " + c + " : " + zero + ") + " + d;
    case gx::TevColorOp::comp_gr16_gt:
      return "((TevPack16(t_TevA.rg) >  TevPack16(t_TevB.rg)) ? " + c + " : " +
             zero + ") + " + d;
    case gx::TevColorOp::comp_gr16_eq:
      return "((TevPack16(t_TevA.rg) == TevPack16(t_TevB.rg)) ? " + c + " : " +
             zero + ") + " + d;
    case gx::TevColorOp::comp_bgr24_gt:
      return "((TevPack24(t_TevA.rgb) >  TevPack24(t_TevB.rgb)) ? " + c +
             " : " + zero + ") + " + d;
    case gx::TevColorOp::comp_bgr24_eq:
      return "((TevPack24(t_TevA.rgb) == TevPack24(t_TevB.rgb)) ? " + c +
             " : " + zero + ") + " + d;
    case gx::TevColorOp::comp_rgb8_gt:
      return std::format("(TevPerCompGT({0}, {1}) * {2}) + {3}", a, b, c, d);
    case gx::TevColorOp::comp_rgb8_eq:
      return std::format("(TevPerCompEQ({0}, {1}) * {2}) + {3}", a, b, c, d);
    }
    return std::unexpected(
        std::format("Invalid TevColorOp{}", static_cast<u32>(op)));
  }

  std::expected<std::string, std::string>
  generateTevOpValue(gx::TevColorOp op, gx::TevBias bias, gx::TevScale scale,
                     bool clamp, const std::string& a, const std::string& b,
                     const std::string& c, const std::string& d,
                     const std::string& zero) {
    const auto expr = TRY(generateTevOp(op, bias, scale, a, b, c, d, zero));

    if (clamp)
      return "TevSaturate(" + expr + ")";
    else
      return expr;
  }

  std::expected<std::string, std::string>
  generateColorOp(const gx::TevStage& stage) {
    const auto a = "t_TevA.rgb", b = "t_TevB.rgb", c = "t_TevC.rgb",
               d = "t_TevD.rgb", zero = "vec3(0)";
    const auto value = TRY(generateTevOpValue(
        stage.colorStage.formula, stage.colorStage.bias, stage.colorStage.scale,
        stage.colorStage.clamp, a, b, c, d, zero));
    return std::format("    {}.rgb = {};\n",
                       TRY(generateTevRegister(stage.colorStage.out)), value);
  }

  std::expected<std::string, std::string>
  generateAlphaOp(const gx::TevStage& stage) {
    const auto a = "t_TevA.a", b = "t_TevB.a", c = "t_TevC.a", d = "t_TevD.a",
               zero = "0.0";
    const auto value = TRY(generateTevOpValue(
        static_cast<gx::TevColorOp>(stage.alphaStage.formula),
        stage.alphaStage.bias, stage.alphaStage.scale, stage.alphaStage.clamp,
        a, b, c, d, zero));
    return std::format("    {}.a = {};\n",
                       TRY(generateTevRegister(stage.alphaStage.out)), value);
  }

  std::expected<std::string, std::string>
  generateTevTexCoordWrapN(const std::string& texCoord, gx::IndTexWrap wrap) {
    switch (wrap) {
    case gx::IndTexWrap::off:
      return texCoord;
    case gx::IndTexWrap::_0:
      return "0.0";
    case gx::IndTexWrap::_256:
      return "mod(" + texCoord + ", 256.0)";
    case gx::IndTexWrap::_128:
      return "mod(" + texCoord + ", 128.0)";
    case gx::IndTexWrap::_64:
      return "mod(" + texCoord + ", 64.0)";
    case gx::IndTexWrap::_32:
      return "mod(" + texCoord + ", 32.0)";
    case gx::IndTexWrap::_16:
      return "mod(" + texCoord + ", 16.0)";
    }
    EXPECT(false, "Invalid enum");
  }

  std::expected<std::string, std::string>
  generateTevTexCoordWrap(const gx::TevStage& stage) {
    const int lastTexGenId = mMaterial.texGens.size() - 1;
    int texGenId = stage.texCoord;

    if (texGenId >= lastTexGenId)
      texGenId = lastTexGenId;
    if (texGenId < 0)
      return "vec2(0.0, 0.0)";

    const auto baseCoord = "ReadTexCoord" + std::to_string(texGenId) + "()";
    if (stage.indirectStage.wrapU == gx::IndTexWrap::off &&
        stage.indirectStage.wrapV == gx::IndTexWrap::off)
      return baseCoord;
    else
      return "vec2(" +
             TRY(generateTevTexCoordWrapN(baseCoord + ".x",
                                          stage.indirectStage.wrapU)) +
             ", " +
             TRY(generateTevTexCoordWrapN(baseCoord + ".y",
                                          stage.indirectStage.wrapV)) +
             ")";
  }

  std::expected<std::string, std::string>
  generateTevTexCoordIndTexCoordBias(const gx::TevStage& stage) {
    const std::string bias =
        (stage.indirectStage.format == gx::IndTexFormat::_8bit) ? "-128.0"
                                                                : "1.0";

    switch (stage.indirectStage.bias) {
    case gx::IndTexBiasSel::none:
      return "";
    case gx::IndTexBiasSel::s:
      return " + vec3(" + bias + ", 0.0, 0.0)";
    case gx::IndTexBiasSel::st:
      return " + vec3(" + bias + ", " + bias + ", 0.0)";
    case gx::IndTexBiasSel::su:
      return " + vec3(" + bias + ", 0.0, " + bias + ")";
    case gx::IndTexBiasSel::t:
      return " + vec3(0.0, " + bias + ", 0.0)";
    case gx::IndTexBiasSel::tu:
      return " + vec3(0.0, " + bias + ", " + bias + ")";
    case gx::IndTexBiasSel::u:
      return " + vec3(0.0, 0.0, " + bias + ")";
    case gx::IndTexBiasSel::stu:
      return " + vec3(" + bias + ")";
    }
    EXPECT(false, "Invalid enum");
  }

  std::expected<std::string, std::string>
  generateTevTexCoordIndTexCoord(const gx::TevStage& stage) {
    const auto baseCoord = "(t_IndTexCoord" +
                           std::to_string(stage.indirectStage.indStageSel) +
                           ")";
    switch (stage.indirectStage.format) {
    case gx::IndTexFormat::_8bit:
      return baseCoord;
    case gx::IndTexFormat::_5bit:
      return std::unexpected("IndTexFmt: 5bit is unsupported");
    case gx::IndTexFormat::_4bit:
      return std::unexpected("IndTexFmt: 4bit is unsupported");
    case gx::IndTexFormat::_3bit:
      return std::unexpected("IndTexFmt: 3bit is unsupported");
    }
    EXPECT(false, "Unsupported IndTexFmt");
  }

  std::expected<std::string, std::string>
  generateTevTexCoordIndirectMtx(const gx::TevStage& stage) {
    const auto indTevCoord = "(" + TRY(generateTevTexCoordIndTexCoord(stage)) +
                             TRY(generateTevTexCoordIndTexCoordBias(stage)) +
                             ")";

    switch (stage.indirectStage.matrix) {
    case gx::IndTexMtxID::_0:
      return "(u_IndTexMtx[0] * vec4(" + indTevCoord + ", 0.0))";
    case gx::IndTexMtxID::_1:
      return "(u_IndTexMtx[1] * vec4(" + indTevCoord + ", 0.0))";
    case gx::IndTexMtxID::_2:
      return "(u_IndTexMtx[2] * vec4(" + indTevCoord + ", 0.0))";
    case gx::IndTexMtxID::off:
      return std::unexpected("IndTexMtxID: off is not supported");
    case gx::IndTexMtxID::s0:
      return std::unexpected("IndTexMtxID: s0 is not supported");
    case gx::IndTexMtxID::s1:
      return std::unexpected("IndTexMtxID: s1 is not supported");
    case gx::IndTexMtxID::s2:
      return std::unexpected("IndTexMtxID: s2 is not supported");
    case gx::IndTexMtxID::t0:
    case gx::IndTexMtxID::t1:
    case gx::IndTexMtxID::t2:
      break;
    }
    return std::unexpected(
        std::format("Unimplemented indTexMatrix mode: {}",
                    static_cast<u32>(stage.indirectStage.matrix)));
  }

  std::expected<std::string, std::string>
  generateTevTexCoordIndirectTranslation(const gx::TevStage& stage) {
    return "(" + TRY(generateTevTexCoordIndirectMtx(stage)) +
           " * TextureInvScale(" + std::to_string(stage.texMap) + "))";
  }

  std::expected<std::string, std::string>
  generateTevTexCoordIndirect(const gx::TevStage& stage) {
    const auto baseCoord = TRY(generateTevTexCoordWrap(stage));

    if (stage.indirectStage.matrix != gx::IndTexMtxID::off &&
        stage.indirectStage.indStageSel < mMaterial.mStages.size())
      return baseCoord + " + " +
             TRY(generateTevTexCoordIndirectTranslation(stage));
    else
      return baseCoord;
  }

  std::expected<std::string, std::string>
  generateTevTexCoord(const gx::TevStage& stage) {
    if (stage.texCoord == 0xff)
      return "";

    const auto finalCoord = TRY(generateTevTexCoordIndirect(stage));
    if (stage.indirectStage.addPrev) {
      return "    t_TexCoord += " + finalCoord + ";\n";
    } else {
      return "    t_TexCoord = " + finalCoord + ";\n";
    }
  }

  Result<void> generateTevStage(StringBuilder& builder, u32 tevStageIndex) {
    const auto& stage = mMaterial.mStages[tevStageIndex];

    builder += "\n\n    //\n    // TEV Stage ";
    builder += std::to_string(tevStageIndex);
    builder += "\n    //\n";
    builder += TRY(generateTevTexCoord(stage));
    builder += TRY(generateTevInputs(stage));
    builder += TRY(generateColorOp(stage));
    builder += TRY(generateAlphaOp(stage));

    return {};
  }

  Result<void> generateTevStages(StringBuilder& builder) {
    for (int i = 0; i < mMaterial.mStages.size(); ++i) {
      TRY(generateTevStage(builder, i));
    }
    return {};
  }

  Result<void> generateTevStagesLastMinuteFixup(StringBuilder& builder) {
    const auto& tevStages = mMaterial.mStages;
    EXPECT(!tevStages.empty());
    const auto& lastTevStage = tevStages[tevStages.size() - 1];
    const auto colorReg = TRY(generateTevRegister(lastTevStage.colorStage.out));
    const auto alphaReg = TRY(generateTevRegister(lastTevStage.alphaStage.out));

    if (colorReg == alphaReg) {
      builder += "    vec4 t_TevOutput = " + colorReg + ";\n";
    } else {
      builder += "    vec4 t_TevOutput = vec4(" + colorReg + ".rgb, " +
                 alphaReg + ".a);\n";
    }

    return {};
  }

  Result<void> generateAlphaTestCompare(StringBuilder& builder,
                                        gx::Comparison compare,
                                        float reference) {
    const auto ref = std::to_string(static_cast<f32>(reference));
    switch (compare) {
    case gx::Comparison::NEVER:
      builder += "false";
      break;
    case gx::Comparison::LESS:
      builder += "t_PixelOut.a <  ";
      builder += ref;
      break;
    case gx::Comparison::EQUAL:
      builder += "t_PixelOut.a == ";
      builder += ref;
      break;
    case gx::Comparison::LEQUAL:
      builder += "t_PixelOut.a <= ";
      builder += ref;
      break;
    case gx::Comparison::GREATER:
      builder += "t_PixelOut.a >  ";
      builder += ref;
      break;
    case gx::Comparison::NEQUAL:
      builder += "t_PixelOut.a != ";
      builder += ref;
      break;
    case gx::Comparison::GEQUAL:
      builder += "t_PixelOut.a >= ";
      builder += ref;
      break;
    case gx::Comparison::ALWAYS:
      builder += "true";
      break;
    }
    return {};
  }

  Result<void> generateAlphaTestOp(StringBuilder& builder, gx::AlphaOp op) {
    switch (op) {
    case gx::AlphaOp::_and:
      builder += "t_AlphaTestA && t_AlphaTestB";
      break;
    case gx::AlphaOp::_or:
      builder += "t_AlphaTestA || t_AlphaTestB";
      break;
    case gx::AlphaOp::_xor:
      builder += "t_AlphaTestA != t_AlphaTestB";
      break;
    case gx::AlphaOp::_xnor:
      builder += "t_AlphaTestA == t_AlphaTestB";
      break;
    }
    return {};
  }
  Result<void> generateAlphaTest(StringBuilder& builder) {
    const auto alphaTest = mMaterial.alphaCompare;
    builder += "\n	bool t_AlphaTestA = ";
    TRY(generateAlphaTestCompare(builder, alphaTest.compLeft,
                                 static_cast<float>(alphaTest.refLeft) /
                                     255.0f));
    builder += ";\n";
    builder += "	bool t_AlphaTestB = ";
    TRY(generateAlphaTestCompare(builder, alphaTest.compRight,
                                 static_cast<float>(alphaTest.refRight) /
                                     255.0f));
    builder += ";\n";
    builder += "	if (!(";
    TRY(generateAlphaTestOp(builder, alphaTest.op));
    builder += "))\n";
    builder += "		discard; \n";

    return {};
  }
  std::string generateFogZCoord() { return ""; }
  std::string generateFogBase() { return ""; }

  std::string generateFogAdj(const std::string& base) { return ""; }

  std::string generateFogFunc(const std::string& base) { return ""; }

  std::string generateFog() { return ""; }
  Result<void> generateAttributeStorageType(StringBuilder& builder,
                                            u32 glFormat, u32 count) {
    // assert(glFormat == GL_FLOAT && "Invalid format");

    switch (count) {
    case 1:
      builder += glFormat == GL_FLOAT          ? "float"
                 : glFormat == GL_UNSIGNED_INT ? "uint"
                                               : "?";
      break;
    case 2:
      builder += "vec2";
      break;
    case 3:
      builder += "vec3";
      break;
    case 4:
      builder += "vec4";
      break;
    default:
      EXPECT(false, "Invalid count");
      break;
    }

    return {};
  }

  Result<void> generateVertAttributeDefs(StringBuilder& builder) {
    int i = 0;
    for (const auto& attr : vtxAttributeGenDefs) {
      // if (attr.format != GL_FLOAT) continue;
      builder += "layout(location = ";
      builder += std::to_string(i);
      builder += ")";

      builder += " in ";
      TRY(generateAttributeStorageType(builder, attr.format, attr.size));
      builder += " a_";
      builder += attr.name;
      builder += ";\n";
      ++i;
    }

    return {};
  }
  Result<void> generateMulPos(StringBuilder& builder) {
    // Default to using pnmtxidx.
    const auto src = "vec4(a_Position, 1.0)";
    if (usePnMtxIdx) {
      builder += generateMulPntMatrixDynamic("uint(a_PnMtxIdx)", src);
    } else {
      TRY(generateMulPntMatrixStatic(builder, gx::PostTexMatrix::Matrix0, src));
    }

    return {};
  }

  Result<void> generateMulNrm(StringBuilder& builder) {
    // Default to using pnmtxidx.
    const auto src = "vec4(a_Normal, 0.0)";
    if (usePnMtxIdx)
      builder += generateMulPntMatrixDynamic("uint(a_PnMtxIdx)", src);
    else {
      TRY(generateMulPntMatrixStatic(builder, gx::PostTexMatrix::Matrix0, src));
    }

    return {};
  }

  std::expected<std::string, std::string> generateVert() {
    const std::string_view varying_vert =
        R"(out vec3 v_Position;
out vec4 v_Color0;
out vec4 v_Color1;
out vec3 v_TexCoord0;
out vec3 v_TexCoord1;
out vec3 v_TexCoord2;
out vec3 v_TexCoord3;
out vec3 v_TexCoord4;
out vec3 v_TexCoord5;
out vec3 v_TexCoord6;
out vec3 v_TexCoord7;
out vec4 v_PrimID;
)";

    std::array<char, 1024 * 64> vert_buf;
    StringBuilder vert(vert_buf.data(), vert_buf.size());
    vert += varying_vert;
    TRY(generateVertAttributeDefs(vert));
    vert += "mat4x3 GetPosTexMatrix(uint t_MtxIdx) {\n"
            "    if (t_MtxIdx == " +
            std::to_string((int)gx::TexMatrix::Identity) +
            "u)\n"
            "        return mat4x3(1.0);\n"
            "    else if (t_MtxIdx >= " +
            std::to_string((int)gx::TexMatrix::TexMatrix0) +
            "u)\n"
            "        return u_TexMtx[(t_MtxIdx - " +
            std::to_string((int)gx::TexMatrix::TexMatrix0) +
            "u) / 3u];\n"
            "    else\n"
            "        return u_PosMtx[t_MtxIdx / 3u];\n"
            "}\n" +
            R"(
float ApplyAttenuation(vec3 t_Coeff, float t_Value) {
    return dot(t_Coeff, vec3(1.0, t_Value, t_Value*t_Value));
}
)";
    vert += "void main() {\n";

    vert += "    vec3 t_Position = ";
    TRY(generateMulPos(vert));
    vert += ";\n";

    vert += "    v_Position = t_Position;\n";
    vert += "    v_PrimID = a_PrimID;";

    vert += "    vec3 t_Normal = ";
    TRY(generateMulNrm(vert));
    vert += ";\n";

    vert += "    vec4 t_LightAccum;\n"
            "    vec3 t_LightDelta, t_LightDeltaDir;\n"
            "    float t_LightDeltaDist2, t_LightDeltaDist, t_Attenuation;\n"
            "    vec4 t_ColorChanTemp;\n"
            "    v_Color0 = a_Color0;\n";
    TRY(generateLightChannels(vert));
    vert += TRY(generateTexGens());
    vert += "gl_Position = (u_Projection * vec4(t_Position, 1.0));\n"
            "}\n";

    return vert_buf.data();
  }

  std::expected<std::string, std::string> generateFrag() {
    constexpr std::string_view varying_frag =
        R"(in vec3 v_Position;
in vec4 v_Color0;
in vec4 v_Color1;
in vec3 v_TexCoord0;
in vec3 v_TexCoord1;
in vec3 v_TexCoord2;
in vec3 v_TexCoord3;
in vec3 v_TexCoord4;
in vec3 v_TexCoord5;
in vec3 v_TexCoord6;
in vec3 v_TexCoord7;
in vec4 v_PrimID;
out vec4 fragOut;
)";

    std::array<char, 1024 * 64> frag_buf;
    StringBuilder frag(frag_buf.data(), frag_buf.size());
#if !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
    if (mMaterial.earlyZComparison) {
      // https://www.khronos.org/opengl/wiki/Early_Fragment_Test#Explicit_specification
      frag += "layout(early_fragment_tests) in;\n";
    }
#endif
    frag += varying_frag;
    frag += generateTexCoordGetters();
    frag += R"(
float TextureLODBias(int index) { return u_SceneTextureLODBias + u_TextureParams[index].w; }
vec2 TextureInvScale(int index) { return 1.0 / u_TextureParams[index].xy; }
vec2 TextureScale(int index) { return u_TextureParams[index].xy; }
vec3 TevBias(vec3 a, float b) { return a + vec3(b); }
float TevBias(float a, float b) { return a + b; }
vec3 TevSaturate(vec3 a) { return clamp(a, vec3(0), vec3(1)); }
float TevSaturate(float a) { return clamp(a, 0.0, 1.0); }
float TevOverflow(float a) { return float(int(a * 255.0) & 255) / 255.0; }
vec4 TevOverflow(vec4 a) { return vec4(TevOverflow(a.r), TevOverflow(a.g), TevOverflow(a.b), TevOverflow(a.a)); }
float TevPack16(vec2 a) { return dot(a, vec2(1.0, 256.0)); }
float TevPack24(vec3 a) { return dot(a, vec3(1.0, 256.0, 256.0 * 256.0)); }
float TevPerCompGT(float a, float b) { return float(a >  b); }
float TevPerCompEQ(float a, float b) { return float(a == b); }
vec3 TevPerCompGT(vec3 a, vec3 b) { return vec3(greaterThan(a, b)); }
vec3 TevPerCompEQ(vec3 a, vec3 b) { return vec3(greaterThan(a, b)); }


void main() {
    vec4 s_kColor0   = u_KonstColor[0];
    vec4 s_kColor1   = u_KonstColor[1];
    vec4 s_kColor2   = u_KonstColor[2];
    vec4 s_kColor3   = u_KonstColor[3];
    vec4 t_ColorPrev = u_Color[0];
    vec4 t_Color0    = u_Color[1];
    vec4 t_Color1    = u_Color[2];
    vec4 t_Color2    = u_Color[3];
)";
    frag += TRY(generateIndTexStages());
    frag +=
        R"(
    vec2 t_TexCoord = vec2(0.0, 0.0);
    vec4 t_TevA, t_TevB, t_TevC, t_TevD;)";
    TRY(generateTevStages(frag));
    TRY(generateTevStagesLastMinuteFixup(frag));
    frag += "    vec4 t_PixelOut = TevOverflow(t_TevOutput);\n";
    TRY(generateAlphaTest(frag));
    frag += generateFog();
    frag += "    fragOut = t_PixelOut;\n";
    if (vis_prim == VisType::PrimID)
      frag += "   fragOut = v_PrimID;\n";
    if (mMaterial.dstAlpha.enabled) {
      frag +=
          std::format("fragOut.a = {};\n",
                      static_cast<float>(mMaterial.dstAlpha.alpha) / 255.0f);
    }
    frag += "}\n";

    return frag_buf.data();
  }

  std::expected<std::string, std::string> generateBoth() {
    const auto bindingsDefinition =
        generateBindingsDefinition(hasPostTexMtxBlock, hasLightsBlock);

#if defined(__EMSCRIPTEN__)
    const std::string version = "#version 300 es";
#elif defined(__APPLE__)
    const std::string version = "#version 400";
#else
    // 420: layout(binding=n) for uniforms
    const std::string version = "#version 420";
#endif

    return version + "\n// " + mName + "\nprecision mediump float;\n" +
           bindingsDefinition;
  }

  std::expected<std::pair<std::string, std::string>, std::string>
  generateShaders() {
    auto both = TRY(generateBoth());
    auto vert = TRY(generateVert());
    auto frag = TRY(generateFrag());

    return std::pair<std::string, std::string>{both + vert, both + frag};
  }

  const gx::LowLevelGxMaterial& mMaterial;

  bool usePnMtxIdx = true;
  bool useTexMtxIdx[16]{false};
  bool hasPostTexMtxBlock = false;
  bool hasLightsBlock = true;

  inline u32 calcParamsBlockSize() const noexcept {
    u32 size = 4 * 2 + 4 * 2 + 4 * 4 + 4 * 4 + 4 * 3 * 10 + 4 * 2 * 3 + 4 * 8;
    if (hasPostTexMtxBlock)
      size += 4 * 3 * 20;
    if (hasLightsBlock)
      size += 4 * 5 * 8;

    return size;
  }
  std::string mName;
  VisType vis_prim = VisType::None;
};

std::expected<GlShaderPair, std::string>
compileShader(const gx::LowLevelGxMaterial& mat, std::string_view name,
              VisType vis_prim) {
  GXProgram program(mat, name);
  program.vis_prim = vis_prim;
  auto compiled = TRY(program.generateShaders());
  return GlShaderPair{compiled.first, compiled.second};
}

} // namespace librii::gl
