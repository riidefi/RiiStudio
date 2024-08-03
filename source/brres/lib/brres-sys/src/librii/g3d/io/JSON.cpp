#include <glm/vec3.hpp>
#include <iostream>
#include <string>
#include <vector>

#include <rsl/Types.hpp>

#include <librii/g3d/data/Archive.hpp>
#include <librii/g3d/data/BoneData.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <librii/g3d/data/ModelData.hpp>
#include <librii/g3d/data/PolygonData.hpp>
#include <librii/g3d/data/VertexData.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#endif

#include <librii/g3d/io/ArchiveIO.hpp>

#include <magic_enum/magic_enum.hpp>

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <rsl/EnumCast.hpp>

// Prevent conflicts with other users of the library
#define JS LIBRII_G3D_IO_JSON_JS

#define JS_STL_ARRAY
#define JS_STD_OPTIONAL
#include <vendor/json_struct.h>

namespace librii::g3d {
struct JsonWriteCtx {
  std::vector<std::vector<u8>> buffers;

  int save_buffer_with_move(std::vector<u8>&& buf) {
    buffers.emplace_back(std::move(buf));
    return static_cast<int>(buffers.size()) - 1;
  }
  template <typename T> int save_buffer_with_copy(std::span<const T> buf) {
    const u8* begin = reinterpret_cast<const u8*>(buf.data());
    std::vector<u8> tmp(begin, begin + buf.size() * sizeof(T));
    return save_buffer_with_move(std::move(tmp));
  }
};
} // namespace librii::g3d

namespace JS {

template <> struct TypeHandler<glm::vec2> {
  static inline Error to(glm::vec2& to_type, ParseContext& context) {
    std::vector<f32> t;
    TypeHandler<std::vector<f32>>::to(t, context);
    to_type.x = t[0];
    to_type.y = t[1];
    return Error::NoError;
  }

  static inline void from(const glm::vec2& vec, Token& token,
                          Serializer& serializer) {
    std::vector<f32> t;
    t.push_back(vec.x);
    t.push_back(vec.y);
    TypeHandler<std::vector<f32>>::from(t, token, serializer);
  }
};

template <> struct TypeHandler<glm::vec3> {
  static inline Error to(glm::vec3& to_type, ParseContext& context) {
    std::vector<f32> t;
    auto err = TypeHandler<std::vector<f32>>::to(t, context);
    if (err != Error::NoError) {
      return err;
    }
    to_type.x = t[0];
    to_type.y = t[1];
    to_type.z = t[2];
    return Error::NoError;
  }

  static inline void from(const glm::vec3& vec, Token& token,
                          Serializer& serializer) {
    std::vector<f32> t;
    t.push_back(vec.x);
    t.push_back(vec.y);
    t.push_back(vec.z);
    TypeHandler<std::vector<f32>>::from(t, token, serializer);
  }
};
template <> struct TypeHandler<glm::vec4> {
  static inline Error to(glm::vec4& to_type, ParseContext& context) {
    std::vector<f32> t;
    auto err = TypeHandler<std::vector<f32>>::to(t, context);
    if (err != Error::NoError) {
      return err;
    }
    to_type.x = t[0];
    to_type.y = t[1];
    to_type.z = t[2];
    to_type.w = t[3];
    return Error::NoError;
  }

  static inline void from(const glm::vec4& vec, Token& token,
                          Serializer& serializer) {
    std::vector<f32> t;
    t.push_back(vec.x);
    t.push_back(vec.y);
    t.push_back(vec.z);
    t.push_back(vec.w);
    TypeHandler<std::vector<f32>>::from(t, token, serializer);
  }
};

} // namespace JS

JS_OBJ_EXT(librii::gx::Color, r, g, b, a);
JS_OBJ_EXT(librii::gx::ColorS10, r, g, b, a);
namespace JS {
template <typename T>
concept IsEnum = std::is_enum_v<T>;
template <IsEnum T> struct TypeHandler<T> {
  static inline Error to(auto& to_type, ParseContext& context) {
    std::string tmp;
    auto err = TypeHandler<std::string>::to(tmp, context);
    if (err != Error::NoError) {
      return err;
    }
    auto res = magic_enum::enum_cast<T>(tmp);
    if (!res) {
      return Error::InvalidToken;
    }
    to_type = *res;
    return Error::NoError;
  }

  static inline void from(T x, Token& token, Serializer& serializer) {
    token.value_type = Type::String;
    std::string_view v = magic_enum::enum_name(x);
    token.value = DataRef(v.data(), v.size());
    serializer.write(token);
  }
};
} // namespace JS

// #include <tser/tser.hpp>

namespace librii::g3d {

#define DEFINE_SERIALIZABLE(T, ...) JS_OBJ(__VA_ARGS__);

struct JSONMatrixWeight {
  DEFINE_SERIALIZABLE(JSONMatrixWeight, bone_id, weight)
  u32 bone_id = 0;
  f32 weight = 1.0f;

  static JSONMatrixWeight from(const DrawMatrix::MatrixWeight& original) {
    return {original.boneId, original.weight};
  }

  operator DrawMatrix::MatrixWeight() const {
    return DrawMatrix::MatrixWeight(bone_id, weight);
  }
};

struct JSONDrawMatrix {
  DEFINE_SERIALIZABLE(JSONDrawMatrix, weights)
  std::vector<JSONMatrixWeight> weights;
  static JSONDrawMatrix from(const DrawMatrix& original) {
    JSONDrawMatrix thing;
    for (const auto& weight : original.mWeights) {
      thing.weights.emplace_back(JSONMatrixWeight::from(weight));
    }
    return thing;
  }

  operator DrawMatrix() const {
    DrawMatrix result;
    for (const auto& jsonWeight : weights) {
      result.mWeights.push_back(
          static_cast<DrawMatrix::MatrixWeight>(jsonWeight));
    }
    return result;
  }
};

struct JSONModelInfo {
  DEFINE_SERIALIZABLE(JSONModelInfo, scaling_rule, texmtx_mode, source_location,
                      evpmtx_mode, min, max)
  std::string scaling_rule = "Maya";
  std::string texmtx_mode = "Maya";
  std::string source_location = "";
  std::string evpmtx_mode = "Normal";
  glm::vec3 min, max;

  static JSONModelInfo from(const ModelInfo& x) {
    JSONModelInfo result;

    result.scaling_rule = magic_enum::enum_name(x.scalingRule);
    result.texmtx_mode = magic_enum::enum_name(x.texMtxMode);
    result.source_location = x.sourceLocation;
    result.evpmtx_mode = magic_enum::enum_name(x.evpMtxMode);
    result.min = x.min;
    result.max = x.max;

    return result;
  }
  Result<ModelInfo> to() const {
    ModelInfo result;

    result.scalingRule =
        TRY(rsl::enum_cast<librii::g3d::ScalingRule>(scaling_rule));
    result.texMtxMode =
        TRY(rsl::enum_cast<librii::g3d::TextureMatrixMode>(texmtx_mode));
    result.sourceLocation = source_location;
    result.evpMtxMode =
        TRY(rsl::enum_cast<librii::g3d::EnvelopeMatrixMode>(evpmtx_mode));
    result.min = min;
    result.max = max;

    return result;
  }
};

struct JSONDrawCall {
  DEFINE_SERIALIZABLE(JSONDrawCall, material, poly, prio)
  u32 material = 0;
  u32 poly = 0;
  int prio = 0;

  static JSONDrawCall from(const BoneData::DisplayCommand& original) {
    return {original.mMaterial, original.mPoly, original.mPrio};
  }

  operator BoneData::DisplayCommand() const {
    return BoneData::DisplayCommand{material, poly, (u8)prio};
  }
};

struct JSONBoneData {
  DEFINE_SERIALIZABLE(JSONBoneData, name, ssc, visible, billboard_type, scale,
                      rotate, translate, volume_min, volume_max, parent,
                      children, display_matrix, draw_calls,
                      force_display_matrix, omit_from_node_mix)

  std::string name = "Untitled Bone";
  bool ssc = false;
  bool visible = true;
  u32 billboard_type = 0;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};
  glm::vec3 rotate = glm::vec3{0.0f, 0.0f, 0.0f};
  glm::vec3 translate = glm::vec3{0.0f, 0.0f, 0.0f};
  glm::vec3 volume_min = glm::vec3{0.0f, 0.0f, 0.0f};
  glm::vec3 volume_max = glm::vec3{0.0f, 0.0f, 0.0f};
  s32 parent = -1;
  std::vector<s32> children;
  bool display_matrix = true;
  std::vector<JSONDrawCall> draw_calls;
  bool force_display_matrix = false;
  bool omit_from_node_mix = false;

  static JSONBoneData from(const BoneData& original) {
    JSONBoneData json;
    json.name = original.mName;
    json.ssc = original.ssc;
    json.visible = original.visible;
    json.billboard_type = original.billboardType;
    json.scale = original.mScaling;
    json.rotate = original.mRotation;
    json.translate = original.mTranslation;
    json.volume_min = original.mVolume.min;
    json.volume_max = original.mVolume.max;
    json.parent = original.mParent;
    json.children = original.mChildren;
    json.display_matrix = original.displayMatrix;
    for (const auto& cmd : original.mDisplayCommands) {
      json.draw_calls.push_back(JSONDrawCall::from(cmd));
    }
    json.force_display_matrix = original.forceDisplayMatrix;
    json.omit_from_node_mix = original.omitFromNodeMix;
    return json;
  }

  operator BoneData() const {
    BoneData bone;
    bone.mName = name;
    bone.ssc = ssc;
    bone.visible = visible;
    bone.billboardType = billboard_type;
    bone.mScaling = scale;
    bone.mRotation = rotate;
    bone.mTranslation = translate;
    bone.mVolume.min = volume_min;
    bone.mVolume.max = volume_max;
    bone.mParent = parent;
    bone.mChildren = children;
    bone.displayMatrix = display_matrix;
    for (const auto& cmd : draw_calls) {
      bone.mDisplayCommands.push_back(cmd);
    }
    bone.forceDisplayMatrix = force_display_matrix;
    bone.omitFromNodeMix = omit_from_node_mix;
    return bone;
  }
};
using namespace librii::gx;

std::vector<u8>
packIndexedPrims(std::span<const librii::gx::IndexedPrimitive> prims) {
  std::vector<u8> vd_buf;

  for (auto& prim : prims) {
    vd_buf.push_back(static_cast<u8>(prim.mType));
    u32 num_verts = prim.mVertices.size();
    vd_buf.push_back((num_verts >> 16) & 0xff);
    vd_buf.push_back((num_verts >> 8) & 0xff);
    vd_buf.push_back((num_verts >> 0) & 0xff);
    // 26x u16s
    static_assert(sizeof(librii::gx::IndexedVertex) == 26 * 2);
    static_assert(alignof(librii::gx::IndexedVertex) == 2);
    u32 cursor = vd_buf.size();
    u32 buf_size = prim.mVertices.size() * sizeof(librii::gx::IndexedVertex);
    vd_buf.resize(vd_buf.size() + buf_size);
    memcpy(vd_buf.data() + cursor, prim.mVertices.data(), buf_size);
  }
  return vd_buf;
}
std::vector<librii::gx::IndexedPrimitive>
unpackIndexedPrims(std::span<const u8> vd_buf, u32 num_prims) {
  std::vector<librii::gx::IndexedPrimitive> prims;
  size_t offset = 0;

  for (u32 i = 0; i < num_prims; ++i) {
    librii::gx::IndexedPrimitive prim;
    prim.mType = static_cast<librii::gx::PrimitiveType>(vd_buf[offset++]);

    u32 num_verts = 0;
    num_verts |= vd_buf[offset++] << 16;
    num_verts |= vd_buf[offset++] << 8;
    num_verts |= vd_buf[offset++];

    prim.mVertices.resize(num_verts);
    size_t buf_size = num_verts * sizeof(librii::gx::IndexedVertex);
    memcpy(prim.mVertices.data(), vd_buf.data() + offset, buf_size);
    offset += buf_size;

    prims.push_back(prim);
  }

  return prims;
}

struct JSONMatrixPrimitive {
  DEFINE_SERIALIZABLE(JSONMatrixPrimitive, matrices, num_prims,
                      vertexDataBufferId)

  std::vector<s16> matrices;
  u32 num_prims = 0;
  u32 vertexDataBufferId = 0;

  static JSONMatrixPrimitive from(const MatrixPrimitive& original,
                                  JsonWriteCtx& ctx) {
    JSONMatrixPrimitive json;
    json.matrices = original.mDrawMatrixIndices;

    json.num_prims = original.mPrimitives.size();
    json.vertexDataBufferId =
        ctx.save_buffer_with_move(packIndexedPrims(original.mPrimitives));
    return json;
  }

  MatrixPrimitive to(std::span<const std::vector<u8>> buffers) const {
    MatrixPrimitive matPrim;
    matPrim.mDrawMatrixIndices = matrices;

    matPrim.mPrimitives =
        unpackIndexedPrims(buffers[vertexDataBufferId], num_prims);
    return matPrim;
  }
};

struct JSONPolygonData {
  DEFINE_SERIALIZABLE(JSONPolygonData, name, current_matrix, visible,
                      pos_buffer, nrm_buffer, clr_buffer, uv_buffer, vcd,
                      mprims)

  // ... PolygonData members
  std::string name;
  s16 current_matrix = -1;
  bool visible = true;
  std::string pos_buffer;
  std::string nrm_buffer;
  std::array<std::string, 2> clr_buffer;
  std::array<std::string, 8> uv_buffer;

  // ... MeshData members
  u32 vcd;
  std::vector<JSONMatrixPrimitive> mprims;

  static JSONPolygonData from(const g3d::PolygonData& original,
                              JsonWriteCtx& c) {
    JSONPolygonData json;

    // Convert PolygonData members
    json.name = original.mName;
    json.current_matrix = original.mCurrentMatrix;
    json.visible = original.visible;
    json.pos_buffer = original.mPositionBuffer;
    json.nrm_buffer = original.mNormalBuffer;
    json.clr_buffer = original.mColorBuffer;
    json.uv_buffer = original.mTexCoordBuffer;

    // Convert MeshData members
    for (const auto& matPrim : original.mMatrixPrimitives) {
      json.mprims.push_back(JSONMatrixPrimitive::from(matPrim, c));
    }
    json.vcd = original.mVertexDescriptor.mBitfield;

    return json;
  }

  g3d::PolygonData to(std::span<const std::vector<u8>> buffers) const {
    g3d::PolygonData poly;

    // Convert JSONPolygonData to PolygonData members
    poly.mName = name;
    poly.mCurrentMatrix = current_matrix;
    poly.visible = visible;
    poly.mPositionBuffer = pos_buffer;
    poly.mNormalBuffer = nrm_buffer;
    poly.mColorBuffer = clr_buffer;
    poly.mTexCoordBuffer = uv_buffer;

    // Convert MeshData members
    for (const auto& jsonMatPrim : mprims) {
      poly.mMatrixPrimitives.push_back(jsonMatPrim.to(buffers));
    }

    for (int i = 0; i < static_cast<int>(librii::gx::VertexAttribute::Max);
         ++i) {
      if ((vcd & (1 << i)) == 0)
        continue;

      // PNMTXIDX is direct u8
      auto fmt = i == 0 ? librii::gx::VertexAttributeType::Direct
                        : librii::gx::VertexAttributeType::Short;
      poly.mVertexDescriptor.mAttributes.emplace(
          static_cast<librii::gx::VertexAttribute>(i), fmt);
    }

    poly.mVertexDescriptor.calcVertexDescriptorFromAttributeList();

    RecomputeMinimalIndexFormat(poly);

    assert(poly.mVertexDescriptor.mBitfield == vcd);

    return poly;
  }
};
struct JSONColorStage {
  DEFINE_SERIALIZABLE(JSONColorStage, constantSelection, a, b, c, d, formula,
                      bias, scale, clamp, out)

  TevKColorSel constantSelection;
  TevColorArg a;
  TevColorArg b;
  TevColorArg c;
  TevColorArg d;
  TevColorOp formula;
  TevBias bias;
  TevScale scale;
  bool clamp;
  TevReg out;

  static JSONColorStage from(const TevStage::ColorStage& original) {
    JSONColorStage json;
    // Direct assignments
    json.constantSelection = original.constantSelection;
    json.a = original.a;
    json.b = original.b;
    json.c = original.c;
    json.d = original.d;
    json.formula = original.formula;
    json.bias = original.bias;
    json.scale = original.scale;
    json.clamp = original.clamp;
    json.out = original.out;
    return json;
  }

  operator TevStage::ColorStage() const {
    TevStage::ColorStage colorStage;
    colorStage.constantSelection = constantSelection;
    colorStage.a = a;
    colorStage.b = b;
    colorStage.c = c;
    colorStage.d = d;
    colorStage.formula = formula;
    colorStage.bias = bias;
    colorStage.scale = scale;
    colorStage.clamp = clamp;
    colorStage.out = out;
    return colorStage;
  }
};
struct JSONAlphaStage {
  DEFINE_SERIALIZABLE(JSONAlphaStage, a, b, c, d, formula, constantSelection,
                      bias, scale, clamp, out)

  TevAlphaArg a;
  TevAlphaArg b;
  TevAlphaArg c;
  TevAlphaArg d;
  TevAlphaOp formula;
  TevKAlphaSel constantSelection;
  TevBias bias;
  TevScale scale;
  bool clamp;
  TevReg out;

  static JSONAlphaStage from(const TevStage::AlphaStage& original) {
    JSONAlphaStage json;
    // Direct assignments
    json.a = original.a;
    json.b = original.b;
    json.c = original.c;
    json.d = original.d;
    json.formula = original.formula;
    json.constantSelection = original.constantSelection;
    json.bias = original.bias;
    json.scale = original.scale;
    json.clamp = original.clamp;
    json.out = original.out;
    return json;
  }

  operator TevStage::AlphaStage() const {
    TevStage::AlphaStage alphaStage;
    alphaStage.a = a;
    alphaStage.b = b;
    alphaStage.c = c;
    alphaStage.d = d;
    alphaStage.formula = formula;
    alphaStage.constantSelection = constantSelection;
    alphaStage.bias = bias;
    alphaStage.scale = scale;
    alphaStage.clamp = clamp;
    alphaStage.out = out;
    return alphaStage;
  }
};
struct JSONIndirectStageTev {
  DEFINE_SERIALIZABLE(JSONIndirectStageTev, indStageSel, format, bias, matrix,
                      wrapU, wrapV, addPrev, utcLod, alpha)

  u8 indStageSel;
  IndTexFormat format;
  IndTexBiasSel bias;
  IndTexMtxID matrix;
  IndTexWrap wrapU;
  IndTexWrap wrapV;
  bool addPrev;
  bool utcLod;
  IndTexAlphaSel alpha;

  static JSONIndirectStageTev from(const TevStage::IndirectStage& original) {
    JSONIndirectStageTev json;
    // Direct assignments
    json.indStageSel = original.indStageSel;
    json.format = original.format;
    json.bias = original.bias;
    json.matrix = original.matrix;
    json.wrapU = original.wrapU;
    json.wrapV = original.wrapV;
    json.addPrev = original.addPrev;
    json.utcLod = original.utcLod;
    json.alpha = original.alpha;
    return json;
  }

  operator TevStage::IndirectStage() const {
    TevStage::IndirectStage indirectStage;
    indirectStage.indStageSel = indStageSel;
    indirectStage.format = format;
    indirectStage.bias = bias;
    indirectStage.matrix = matrix;
    indirectStage.wrapU = wrapU;
    indirectStage.wrapV = wrapV;
    indirectStage.addPrev = addPrev;
    indirectStage.utcLod = utcLod;
    indirectStage.alpha = alpha;
    return indirectStage;
  }
};
struct JSONTevStage {
  DEFINE_SERIALIZABLE(JSONTevStage, rasOrder, texMap, texCoord, rasSwap,
                      texMapSwap, colorStage, alphaStage, indirectStage)

  ColorSelChanApi rasOrder;
  u8 texMap;
  u8 texCoord;
  u8 rasSwap;
  u8 texMapSwap;
  JSONColorStage colorStage;
  JSONAlphaStage alphaStage;
  JSONIndirectStageTev indirectStage;

  static JSONTevStage from(const TevStage& original) {
    JSONTevStage json;
    json.rasOrder = original.rasOrder;
    json.texMap = original.texMap;
    json.texCoord = original.texCoord;
    json.rasSwap = original.rasSwap;
    json.texMapSwap = original.texMapSwap;
    json.colorStage = JSONColorStage::from(original.colorStage);
    json.alphaStage = JSONAlphaStage::from(original.alphaStage);
    json.indirectStage = JSONIndirectStageTev::from(original.indirectStage);
    return json;
  }

  operator TevStage() const {
    TevStage stage;
    stage.rasOrder = rasOrder;
    stage.texMap = texMap;
    stage.texCoord = texCoord;
    stage.rasSwap = rasSwap;
    stage.texMapSwap = texMapSwap;
    stage.colorStage = colorStage;
    stage.alphaStage = alphaStage;
    stage.indirectStage = indirectStage;
    return stage;
  }
};

struct JSONTexMatrix {
  DEFINE_SERIALIZABLE(JSONTexMatrix, projection, scale, rotate, translate,
                      effectMatrix, transformModel, method, option, camIdx,
                      lightIdx)

  gx::TexGenType projection;
  glm::vec2 scale;
  f32 rotate;
  glm::vec2 translate;
  std::array<f32, 16> effectMatrix;
  mtx::CommonTransformModel transformModel;
  mtx::CommonMappingMethod method;
  mtx::CommonMappingOption option;
  s8 camIdx;
  s8 lightIdx;

  static JSONTexMatrix from(const GCMaterialData::TexMatrix& original) {
    JSONTexMatrix json;
    // Assign the members directly...
    json.projection = original.projection;
    json.scale = original.scale;
    json.rotate = original.rotate;
    json.translate = original.translate;
    json.effectMatrix = original.effectMatrix;
    json.transformModel = original.transformModel;
    json.method = original.method;
    json.option = original.option;
    json.camIdx = original.camIdx;
    json.lightIdx = original.lightIdx;
    return json;
  }

  operator GCMaterialData::TexMatrix() const {
    GCMaterialData::TexMatrix matrix;
    matrix.projection = projection;
    matrix.scale = scale;
    matrix.rotate = rotate;
    matrix.translate = translate;
    matrix.effectMatrix = effectMatrix;
    matrix.transformModel = transformModel;
    matrix.method = method;
    matrix.option = option;
    matrix.camIdx = camIdx;
    matrix.lightIdx = lightIdx;
    return matrix;
  }
};

struct JSONSamplerData {
  DEFINE_SERIALIZABLE(JSONSamplerData, mTexture, mPalette, mWrapU, mWrapV,
                      bEdgeLod, bBiasClamp, mMaxAniso, mMinFilter, mMagFilter,
                      mLodBias, btiId)

  std::string mTexture;
  std::string mPalette;
  gx::TextureWrapMode mWrapU;
  gx::TextureWrapMode mWrapV;
  bool bEdgeLod;
  bool bBiasClamp;
  gx::AnisotropyLevel mMaxAniso;
  gx::TextureFilter mMinFilter;
  gx::TextureFilter mMagFilter;
  f32 mLodBias;
  u16 btiId;

  static JSONSamplerData from(const GCMaterialData::SamplerData& original) {
    JSONSamplerData json;
    json.mTexture = original.mTexture;
    json.mPalette = original.mPalette;
    json.mWrapU = original.mWrapU;
    json.mWrapV = original.mWrapV;
    json.bEdgeLod = original.bEdgeLod;
    json.bBiasClamp = original.bBiasClamp;
    json.mMaxAniso = original.mMaxAniso;
    json.mMinFilter = original.mMinFilter;
    json.mMagFilter = original.mMagFilter;
    json.mLodBias = original.mLodBias;
    json.btiId = original.btiId;
    return json;
  }

  operator GCMaterialData::SamplerData() const {
    GCMaterialData::SamplerData data;
    data.mTexture = mTexture;
    data.mPalette = mPalette;
    data.mWrapU = mWrapU;
    data.mWrapV = mWrapV;
    data.bEdgeLod = bEdgeLod;
    data.bBiasClamp = bBiasClamp;
    data.mMaxAniso = mMaxAniso;
    data.mMinFilter = mMinFilter;
    data.mMagFilter = mMagFilter;
    data.mLodBias = mLodBias;
    data.btiId = btiId;
    return data;
  }
};
struct JSONChannelControl {
  DEFINE_SERIALIZABLE(JSONChannelControl, enabled, Ambient, Material, lightMask,
                      diffuseFn, attenuationFn)

  bool enabled;
  ColorSource Ambient;
  ColorSource Material;
  LightID lightMask;
  DiffuseFunction diffuseFn;
  AttenuationFunction attenuationFn;

  static JSONChannelControl from(const ChannelControl& original) {
    JSONChannelControl json;
    json.enabled = original.enabled;
    json.Ambient = original.Ambient;
    json.Material = original.Material;
    json.lightMask = original.lightMask;
    json.diffuseFn = original.diffuseFn;
    json.attenuationFn = original.attenuationFn;
    return json;
  }

  operator ChannelControl() const {
    ChannelControl control;
    control.enabled = enabled;
    control.Ambient = Ambient;
    control.Material = Material;
    control.lightMask = lightMask;
    control.diffuseFn = diffuseFn;
    control.attenuationFn = attenuationFn;
    return control;
  }
};

struct JSONChannelData {
  DEFINE_SERIALIZABLE(JSONChannelData, matColor, ambColor)

  gx::Color matColor;
  gx::Color ambColor;

  static JSONChannelData from(const ChannelData& original) {
    JSONChannelData json;
    json.matColor = original.matColor;
    json.ambColor = original.ambColor;
    return json;
  }

  operator ChannelData() const {
    ChannelData data;
    data.matColor = matColor;
    data.ambColor = ambColor;
    return data;
  }
};
struct JSONTexCoordGen {
  DEFINE_SERIALIZABLE(JSONTexCoordGen, func, sourceParam, matrix, normalize,
                      postMatrix)

  TexGenType func;
  TexGenSrc sourceParam;
  TexMatrix matrix;
  bool normalize;
  PostTexMatrix postMatrix;

  static JSONTexCoordGen from(const TexCoordGen& original) {
    JSONTexCoordGen json;
    json.func = original.func;
    json.sourceParam = original.sourceParam;
    json.matrix = original.matrix;
    json.normalize = original.normalize;
    json.postMatrix = original.postMatrix;
    return json;
  }
  operator TexCoordGen() const {
    TexCoordGen texCoordGen;
    texCoordGen.func = func;
    texCoordGen.sourceParam = sourceParam;
    texCoordGen.matrix = matrix;
    texCoordGen.normalize = normalize;
    texCoordGen.postMatrix = postMatrix;
    return texCoordGen;
  }
};
struct JSONIndirectMatrix {
  DEFINE_SERIALIZABLE(JSONIndirectMatrix, scale, rotate, trans, quant, method,
                      refLight)

  glm::vec2 scale;
  f32 rotate;
  glm::vec2 trans;
  int quant;
  IndirectMatrix::Method method;
  s8 refLight;

  static JSONIndirectMatrix from(const IndirectMatrix& original) {
    JSONIndirectMatrix json;
    json.scale = original.scale;
    json.rotate = original.rotate;
    json.trans = original.trans;
    json.quant = original.quant;
    json.method = original.method;
    json.refLight = original.refLight;
    return json;
  }

  operator IndirectMatrix() const {
    IndirectMatrix matrix;
    matrix.scale = scale;
    matrix.rotate = rotate;
    matrix.trans = trans;
    matrix.quant = quant;
    matrix.method = method;
    matrix.refLight = refLight;
    return matrix;
  }
};

struct JSONIndOrder {
  DEFINE_SERIALIZABLE(JSONIndOrder, refMap, refCoord)

  u8 refMap;
  u8 refCoord;

  static JSONIndOrder from(const IndOrder& original) {
    JSONIndOrder json;
    json.refMap = original.refMap;
    json.refCoord = original.refCoord;
    return json;
  }

  operator IndOrder() const {
    IndOrder order;
    order.refMap = refMap;
    order.refCoord = refCoord;
    return order;
  }
};
struct JSONIndirectTextureScalePair {
  DEFINE_SERIALIZABLE(JSONIndirectTextureScalePair, U, V)

  IndirectTextureScalePair::Selection U;
  IndirectTextureScalePair::Selection V;

  static JSONIndirectTextureScalePair
  from(const IndirectTextureScalePair& original) {
    JSONIndirectTextureScalePair json;
    json.U = original.U;
    json.V = original.V;
    return json;
  }

  operator IndirectTextureScalePair() const {
    IndirectTextureScalePair pair;
    pair.U = U;
    pair.V = V;
    return pair;
  }
};
struct JSONIndirectStage {
  DEFINE_SERIALIZABLE(JSONIndirectStage, scale, order)

  JSONIndirectTextureScalePair scale;
  JSONIndOrder order;

  static JSONIndirectStage from(const IndirectStage& original) {
    JSONIndirectStage json;
    json.scale = JSONIndirectTextureScalePair::from(original.scale);
    json.order = JSONIndOrder::from(original.order);
    return json;
  }

  operator IndirectStage() const {
    IndirectStage stage;
    stage.scale = scale;
    stage.order = order;
    return stage;
  }
};

struct JSONZMode {
  DEFINE_SERIALIZABLE(JSONZMode, compare, function, update)

  bool compare;
  Comparison function;
  bool update;

  static JSONZMode from(const ZMode& original) {
    JSONZMode json;
    json.compare = original.compare;
    json.function = original.function;
    json.update = original.update;
    return json;
  }

  operator ZMode() const {
    ZMode zmode;
    zmode.compare = compare;
    zmode.function = function;
    zmode.update = update;
    return zmode;
  }
};

struct JSONAlphaComparison {
  DEFINE_SERIALIZABLE(JSONAlphaComparison, compLeft, refLeft, op, compRight,
                      refRight)

  Comparison compLeft;
  u8 refLeft;
  AlphaOp op;
  Comparison compRight;
  u8 refRight;

  static JSONAlphaComparison from(const AlphaComparison& original) {
    JSONAlphaComparison json;
    json.compLeft = original.compLeft;
    json.refLeft = original.refLeft;
    json.op = original.op;
    json.compRight = original.compRight;
    json.refRight = original.refRight;
    return json;
  }

  operator AlphaComparison() const {
    AlphaComparison alphaComparison;
    alphaComparison.compLeft = compLeft;
    alphaComparison.refLeft = refLeft;
    alphaComparison.op = op;
    alphaComparison.compRight = compRight;
    alphaComparison.refRight = refRight;
    return alphaComparison;
  }
};

struct JSONBlendMode {
  DEFINE_SERIALIZABLE(JSONBlendMode, type, source, dest, logic)

  BlendModeType type;
  BlendModeFactor source;
  BlendModeFactor dest;
  LogicOp logic;

  static JSONBlendMode from(const BlendMode& original) {
    JSONBlendMode json;
    json.type = original.type;
    json.source = original.source;
    json.dest = original.dest;
    json.logic = original.logic;
    return json;
  }

  operator BlendMode() const {
    BlendMode blendMode;
    blendMode.type = type;
    blendMode.source = source;
    blendMode.dest = dest;
    blendMode.logic = logic;
    return blendMode;
  }
};

struct JSONDstAlpha {
  DEFINE_SERIALIZABLE(JSONDstAlpha, enabled, alpha)

  bool enabled;
  u8 alpha;

  static JSONDstAlpha from(const DstAlpha& original) {
    JSONDstAlpha json;
    json.enabled = original.enabled;
    json.alpha = original.alpha;
    return json;
  }

  operator DstAlpha() const {
    DstAlpha dstAlpha;
    dstAlpha.enabled = enabled;
    dstAlpha.alpha = alpha;
    return dstAlpha;
  }
};

struct JSONSwapTableEntry {
  DEFINE_SERIALIZABLE(JSONSwapTableEntry, r, g, b, a)

  ColorComponent r;
  ColorComponent g;
  ColorComponent b;
  ColorComponent a;

  static JSONSwapTableEntry from(const SwapTableEntry& original) {
    JSONSwapTableEntry json;
    json.r = original.r;
    json.g = original.g;
    json.b = original.b;
    json.a = original.a;
    return json;
  }

  operator SwapTableEntry() const {
    SwapTableEntry entry;
    entry.r = r;
    entry.g = g;
    entry.b = b;
    entry.a = a;
    return entry;
  }
};

struct JSONSwapTable {
  DEFINE_SERIALIZABLE(JSONSwapTable, entries)

  std::array<JSONSwapTableEntry, 4> entries;

  static JSONSwapTable from(const SwapTable& original) {
    JSONSwapTable json;
    for (size_t i = 0; i < 4; ++i) {
      json.entries[i] = JSONSwapTableEntry::from(original[i]);
    }
    return json;
  }

  operator SwapTable() const {
    SwapTable table;
    for (size_t i = 0; i < 4; ++i) {
      table[i] = entries[i];
    }
    return table;
  }
};
struct JSONG3dMaterialData {
  DEFINE_SERIALIZABLE(JSONG3dMaterialData, flag, id, lightSetIndex, fogIndex,
                      name, texMatrices, samplers, cullMode, chanData,
                      colorChanControls, texGens, tevKonstColors, tevColors,
                      earlyZComparison, zMode, alphaCompare, blendMode,
                      dstAlpha, xlu, indirectStages, mIndMatrices, mSwapTable,
                      mStages)

  u32 flag;
  u32 id;
  s8 lightSetIndex;
  s8 fogIndex;

  // GCMaterialData attributes
  std::string name;
  std::vector<JSONTexMatrix> texMatrices; // MAX 10
  std::vector<JSONSamplerData> samplers;  // MAX 8

  // LowLevelGxMaterial attributes
  CullMode cullMode;
  std::vector<JSONChannelData> chanData;             // Max 2
  std::vector<JSONChannelControl> colorChanControls; // Max 4
  std::vector<JSONTexCoordGen> texGens;              // Max 8
  std::array<Color, 4> tevKonstColors;
  std::array<ColorS10, 4> tevColors;
  bool earlyZComparison;
  JSONZMode zMode;
  JSONAlphaComparison alphaCompare;
  JSONBlendMode blendMode;
  JSONDstAlpha dstAlpha;
  bool xlu;
  std::vector<JSONIndirectStage> indirectStages; // Max 4
  std::vector<JSONIndirectMatrix> mIndMatrices;  // Max 3
  JSONSwapTable mSwapTable;
  std::vector<JSONTevStage> mStages; // Max 16

  static JSONG3dMaterialData from(const G3dMaterialData& original) {
    JSONG3dMaterialData json;
    json.flag = original.flag;
    json.id = original.id;
    json.lightSetIndex = original.lightSetIndex;
    json.fogIndex = original.fogIndex;
    json.name = original.name;

    for (const auto& texMatrix : original.texMatrices) {
      json.texMatrices.push_back(JSONTexMatrix::from(texMatrix));
    }

    for (const auto& sampler : original.samplers) {
      json.samplers.push_back(JSONSamplerData::from(sampler));
    }

    // Assigning LowLevelGxMaterial members
    json.cullMode = original.cullMode;
    for (const auto& x : original.chanData) {
      json.chanData.push_back(JSONChannelData::from(x));
    }
    for (const auto& x : original.colorChanControls) {
      json.colorChanControls.push_back(JSONChannelControl::from(x));
    }
    for (const auto& x : original.texGens) {
      json.texGens.push_back(JSONTexCoordGen::from(x));
    }
    json.tevKonstColors = original.tevKonstColors;
    json.tevColors = original.tevColors;
    json.earlyZComparison = original.earlyZComparison;
    json.zMode = JSONZMode::from(original.zMode);
    json.alphaCompare = JSONAlphaComparison::from(original.alphaCompare);
    json.blendMode = JSONBlendMode::from(original.blendMode);
    json.dstAlpha = JSONDstAlpha::from(original.dstAlpha);
    json.xlu = original.xlu;

    for (const auto& stage : original.indirectStages) {
      json.indirectStages.push_back(JSONIndirectStage::from(stage));
    }
    for (const auto& matrix : original.mIndMatrices) {
      json.mIndMatrices.push_back(JSONIndirectMatrix::from(matrix));
    }
    json.mSwapTable = JSONSwapTable::from(original.mSwapTable);

    for (const auto& stage : original.mStages) {
      json.mStages.push_back(JSONTevStage::from(stage));
    }

    return json;
  }

  operator G3dMaterialData() const {
    G3dMaterialData material;
    material.flag = flag;
    material.id = id;
    material.lightSetIndex = lightSetIndex;
    material.fogIndex = fogIndex;
    material.name = name;

    for (const auto& jsonTexMatrix : texMatrices) {
      material.texMatrices.push_back(jsonTexMatrix);
    }

    for (const auto& jsonSampler : samplers) {
      material.samplers.push_back(jsonSampler);
    }

    // Extracting LowLevelGxMaterial members
    material.cullMode = cullMode;
    for (const auto& x : chanData) {
      material.chanData.push_back(x);
    }
    for (const auto& x : colorChanControls) {
      material.colorChanControls.push_back(x);
    }
    for (const auto& x : texGens) {
      material.texGens.push_back(x);
    }
    material.tevKonstColors = tevKonstColors;
    material.tevColors = tevColors;
    material.earlyZComparison = earlyZComparison;
    material.zMode = zMode;
    material.alphaCompare = alphaCompare;
    material.blendMode = blendMode;
    material.dstAlpha = dstAlpha;
    material.xlu = xlu;

    material.indirectStages.resize(0);
    for (const auto& jsonStage : indirectStages) {
      material.indirectStages.push_back(jsonStage);
    }
    material.mIndMatrices.resize(0);
    for (const auto& jsonMatrix : mIndMatrices) {
      material.mIndMatrices.push_back(jsonMatrix);
    }
    material.mSwapTable = mSwapTable;

    material.mStages.resize(0);
    for (const auto& jsonStage : mStages) {
      material.mStages.push_back(jsonStage);
    }

    return material;
  }
};

// Utility function to pack entries into a vector of bytes
template <typename T>
std::vector<u8> packEntries(const std::vector<T>& entries) {
  std::vector<u8> buffer(entries.size() * sizeof(T));
  std::memcpy(buffer.data(), entries.data(), buffer.size());
  return buffer;
}

// Utility function to unpack entries from a vector of bytes
template <typename T> std::vector<T> unpackEntries(std::span<const u8> buffer) {
  std::vector<T> entries(buffer.size() / sizeof(T));
  std::memcpy(entries.data(), buffer.data(), buffer.size());
  return entries;
}

struct JSONPositionBuffer {
  DEFINE_SERIALIZABLE(JSONPositionBuffer, name, id, q_type, q_divisor, q_stride,
                      q_comp, dataBufferId, cached_min, cached_max)

  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Generic */ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::Position */ u32 q_comp;
  u32 dataBufferId;

  std::optional<glm::vec3> cached_min;
  std::optional<glm::vec3> cached_max;

  static JSONPositionBuffer from(const PositionBuffer& original,
                                 JsonWriteCtx& ctx) {
    JSONPositionBuffer json;
    json.name = original.mName;
    json.id = original.mId;
    json.q_type = static_cast<u32>(original.mQuantize.mType.generic);
    json.q_divisor = original.mQuantize.divisor;
    json.q_stride = original.mQuantize.stride;
    json.q_comp = static_cast<u32>(original.mQuantize.mComp.position);
    json.dataBufferId =
        ctx.save_buffer_with_move(packEntries(original.mEntries));
    if (original.mCachedMinMax) {
      json.cached_min = original.mCachedMinMax->min;
      json.cached_max = original.mCachedMinMax->max;
    }
    return json;
  }

  PositionBuffer to(std::span<const std::vector<u8>> buffers) const {
    PositionBuffer buffer;
    buffer.mName = name;
    buffer.mId = id;
    buffer.mQuantize.mType = VertexBufferType(
        static_cast<librii::gx::VertexBufferType::Generic>(q_type));
    buffer.mQuantize.divisor = q_divisor;
    buffer.mQuantize.stride = q_stride;
    buffer.mQuantize.mComp =
        VertexComponentCount(static_cast<VCC::Position>(q_comp));
    buffer.mEntries = unpackEntries<glm::vec3>(buffers[dataBufferId]);
    buffer.mCachedMinMax = std::nullopt;
    if (cached_min && cached_max) {
      buffer.mCachedMinMax = {*cached_min, *cached_max};
    }
    return buffer;
  }
};

struct JSONNormalBuffer {
  DEFINE_SERIALIZABLE(JSONNormalBuffer, name, id, q_type, q_divisor, q_stride,
                      q_comp, dataBufferId, cached_min, cached_max)

  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Generic */ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::Normal */ u32 q_comp;
  u32 dataBufferId;

  std::optional<glm::vec3> cached_min;
  std::optional<glm::vec3> cached_max;

  static JSONNormalBuffer from(const NormalBuffer& original,
                               JsonWriteCtx& ctx) {
    JSONNormalBuffer json;
    json.name = original.mName;
    json.id = original.mId;
    json.q_type = static_cast<u32>(original.mQuantize.mType.generic);
    json.q_divisor = original.mQuantize.divisor;
    json.q_stride = original.mQuantize.stride;
    json.q_comp = static_cast<u32>(original.mQuantize.mComp.normal);
    json.dataBufferId =
        ctx.save_buffer_with_move(packEntries(original.mEntries));
    if (original.mCachedMinMax) {
      json.cached_min = original.mCachedMinMax->min;
      json.cached_max = original.mCachedMinMax->max;
    }
    return json;
  }

  NormalBuffer to(std::span<const std::vector<u8>> buffers) const {
    NormalBuffer buffer;
    buffer.mName = name;
    buffer.mId = id;
    buffer.mQuantize.mType = VertexBufferType(
        static_cast<librii::gx::VertexBufferType::Generic>(q_type));
    buffer.mQuantize.divisor = q_divisor;
    buffer.mQuantize.stride = q_stride;
    buffer.mQuantize.mComp =
        VertexComponentCount(static_cast<VCC::Normal>(q_comp));
    buffer.mEntries = unpackEntries<glm::vec3>(buffers[dataBufferId]);
    buffer.mCachedMinMax = std::nullopt;
    if (cached_min && cached_max) {
      buffer.mCachedMinMax = {*cached_min, *cached_max};
    }
    return buffer;
  }
};

struct JSONColorBuffer {
  DEFINE_SERIALIZABLE(JSONColorBuffer, name, id, q_type, q_divisor, q_stride,
                      q_comp, dataBufferId, cached_min, cached_max)

  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Color*/ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::Color*/ u32 q_comp;
  u32 dataBufferId;

  std::optional<std::array<u8, 4>> cached_min;
  std::optional<std::array<u8, 4>> cached_max;

  static JSONColorBuffer from(const ColorBuffer& original, JsonWriteCtx& ctx) {
    JSONColorBuffer json;
    json.name = original.mName;
    json.id = original.mId;
    json.q_type = static_cast<u32>(original.mQuantize.mType.color);
    json.q_divisor = original.mQuantize.divisor;
    json.q_stride = original.mQuantize.stride;
    json.q_comp = static_cast<u32>(original.mQuantize.mComp.color);
    json.dataBufferId =
        ctx.save_buffer_with_move(packEntries(original.mEntries));
    if (original.mCachedMinMax) {
      json.cached_min = original.mCachedMinMax->min.to_array();
      json.cached_max = original.mCachedMinMax->max.to_array();
    }
    return json;
  }

  ColorBuffer to(std::span<const std::vector<u8>> buffers) const {
    ColorBuffer buffer;
    buffer.mName = name;
    buffer.mId = id;
    buffer.mQuantize.mType = VertexBufferType(
        static_cast<librii::gx::VertexBufferType::Color>(q_type));
    buffer.mQuantize.divisor = q_divisor;
    buffer.mQuantize.stride = q_stride;
    buffer.mQuantize.mComp =
        VertexComponentCount(static_cast<VCC::Color>(q_comp));
    buffer.mEntries = unpackEntries<librii::gx::Color>(buffers[dataBufferId]);
    buffer.mCachedMinMax = std::nullopt;
    if (cached_min && cached_max) {
      buffer.mCachedMinMax = {librii::gx::Color(*cached_min),
                              librii::gx::Color(*cached_max)};
    }
    return buffer;
  }
};

struct JSONTextureCoordinateBuffer {
  DEFINE_SERIALIZABLE(JSONTextureCoordinateBuffer, name, id, q_type, q_divisor,
                      q_stride, q_comp, dataBufferId, cached_min, cached_max)
  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Generic */ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::TextureCoordinate */ u32 q_comp;
  u32 dataBufferId;

  std::optional<glm::vec2> cached_min;
  std::optional<glm::vec2> cached_max;

  static JSONTextureCoordinateBuffer
  from(const TextureCoordinateBuffer& original, JsonWriteCtx& ctx) {
    JSONTextureCoordinateBuffer json;
    json.name = original.mName;
    json.id = original.mId;
    json.q_type = static_cast<u32>(original.mQuantize.mType.generic);
    json.q_divisor = original.mQuantize.divisor;
    json.q_stride = original.mQuantize.stride;
    json.q_comp = static_cast<u32>(original.mQuantize.mComp.texcoord);
    json.dataBufferId =
        ctx.save_buffer_with_move(packEntries(original.mEntries));
    if (original.mCachedMinMax) {
      json.cached_min = original.mCachedMinMax->min;
      json.cached_max = original.mCachedMinMax->max;
    }
    return json;
  }

  TextureCoordinateBuffer to(std::span<const std::vector<u8>> buffers) const {
    TextureCoordinateBuffer buffer;
    buffer.mName = name;
    buffer.mId = id;
    buffer.mQuantize.mType = VertexBufferType(
        static_cast<librii::gx::VertexBufferType::Generic>(q_type));
    buffer.mQuantize.divisor = q_divisor;
    buffer.mQuantize.stride = q_stride;
    buffer.mQuantize.mComp =
        VertexComponentCount(static_cast<VCC::TextureCoordinate>(q_comp));
    buffer.mEntries = unpackEntries<glm::vec2>(buffers[dataBufferId]);
    buffer.mCachedMinMax = std::nullopt;
    if (cached_min && cached_max) {
      buffer.mCachedMinMax = {*cached_min, *cached_max};
    }
    return buffer;
  }
};

struct JSONModel {
  DEFINE_SERIALIZABLE(JSONModel, name, info, bones, positions, normals, colors,
                      texcoords, materials, meshes, matrices)

  std::string name = "";
  JSONModelInfo info;
  std::vector<JSONBoneData> bones;
  std::vector<JSONPositionBuffer> positions;
  std::vector<JSONNormalBuffer> normals;
  std::vector<JSONColorBuffer> colors;
  std::vector<JSONTextureCoordinateBuffer> texcoords;
  std::vector<JSONG3dMaterialData> materials;
  std::vector<JSONPolygonData> meshes;
  std::vector<JSONDrawMatrix> matrices;

  static JSONModel from(const Model& original, JsonWriteCtx& c) {
    JSONModel model;
    model.name = original.name;
    model.info = JSONModelInfo::from(original.info);

    for (const auto& bone : original.bones) {
      model.bones.emplace_back(JSONBoneData::from(bone));
    }

    for (const auto& position : original.positions) {
      model.positions.emplace_back(JSONPositionBuffer::from(position, c));
    }

    for (const auto& normal : original.normals) {
      model.normals.emplace_back(JSONNormalBuffer::from(normal, c));
    }

    for (const auto& color : original.colors) {
      model.colors.emplace_back(JSONColorBuffer::from(color, c));
    }

    for (const auto& texcoord : original.texcoords) {
      model.texcoords.emplace_back(
          JSONTextureCoordinateBuffer::from(texcoord, c));
    }

    for (const auto& material : original.materials) {
      model.materials.emplace_back(JSONG3dMaterialData::from(material));
    }

    for (const auto& mesh : original.meshes) {
      model.meshes.emplace_back(JSONPolygonData::from(mesh, c));
    }

    for (const auto& matrix : original.matrices) {
      model.matrices.emplace_back(JSONDrawMatrix::from(matrix));
    }

    return model;
  }

  Result<Model> lift(std::span<const std::vector<u8>> buffers) const {
    Model result;
    result.name = name;
    result.info = TRY(info.to());
    for (const auto& jsonBone : bones) {
      result.bones.push_back(static_cast<BoneData>(jsonBone));
    }

    for (const auto& jsonPosition : positions) {
      result.positions.push_back(jsonPosition.to(buffers));
    }

    for (const auto& jsonNormal : normals) {
      result.normals.push_back(jsonNormal.to(buffers));
    }

    for (const auto& jsonColor : colors) {
      result.colors.push_back(jsonColor.to(buffers));
    }

    for (const auto& jsonTexCoord : texcoords) {
      result.texcoords.push_back(jsonTexCoord.to(buffers));
    }

    for (const auto& jsonMaterial : materials) {
      result.materials.push_back(static_cast<G3dMaterialData>(jsonMaterial));
    }

    for (const auto& jsonMesh : meshes) {
      result.meshes.push_back(jsonMesh.to(buffers));
    }

    for (const auto& jsonMatrix : matrices) {
      result.matrices.push_back(static_cast<DrawMatrix>(jsonMatrix));
    }

    return result;
  }
};

struct JSONTextureData {
  DEFINE_SERIALIZABLE(JSONTextureData, name, format, width, height,
                      number_of_images, minLod, maxLod, sourcePath,
                      dataBufferId)
  std::string name;
  int format;
  u32 width;
  u32 height;
  u32 number_of_images;
  float minLod;
  float maxLod;
  std::string sourcePath;
  u32 dataBufferId;

  static JSONTextureData from(const librii::g3d::TextureData& tex,
                              JsonWriteCtx& ctx) {
    JSONTextureData json;
    json.name = tex.name;
    json.format = static_cast<int>(tex.format);
    json.width = tex.width;
    json.height = tex.height;
    json.number_of_images = tex.number_of_images;
    json.minLod = tex.minLod;
    json.maxLod = tex.maxLod;
    json.sourcePath = tex.sourcePath;
    json.dataBufferId = ctx.save_buffer_with_copy<u8>(tex.data);
    return json;
  }

  librii::g3d::TextureData to(std::span<const std::vector<u8>> buffers) const {
    librii::g3d::TextureData tex;
    tex.name = name;
    tex.format = static_cast<librii::gx::TextureFormat>(format);
    tex.width = width;
    tex.height = height;
    tex.number_of_images = number_of_images;
    tex.minLod = minLod;
    tex.maxLod = maxLod;
    tex.sourcePath = sourcePath;
    tex.data = buffers[dataBufferId];
    return tex;
  }
};

struct JSONSrtKeyFrame {
  float frame{};
  float value{};
  float tangent{};

  DEFINE_SERIALIZABLE(JSONSrtKeyFrame, frame, value, tangent)

  static JSONSrtKeyFrame from(const SRT0KeyFrame& keyFrame) {
    return {keyFrame.frame, keyFrame.value, keyFrame.tangent};
  }

  SRT0KeyFrame to() const { return {frame, value, tangent}; }
};

struct JSONSrtTrack {
  std::vector<JSONSrtKeyFrame> keyframes;

  DEFINE_SERIALIZABLE(JSONSrtTrack, keyframes)

  static JSONSrtTrack from(const SrtAnim::Track& track) {
    JSONSrtTrack jsonTrack;
    for (const auto& keyFrame : track) {
      jsonTrack.keyframes.push_back(JSONSrtKeyFrame::from(keyFrame));
    }
    return jsonTrack;
  }

  SrtAnim::Track to() const {
    SrtAnim::Track track;
    for (const auto& jsonKeyFrame : keyframes) {
      track.push_back(jsonKeyFrame.to());
    }
    return track;
  }
};

struct JSONSrtMatrix {
  JSONSrtTrack scaleX;
  JSONSrtTrack scaleY;
  JSONSrtTrack rot;
  JSONSrtTrack transX;
  JSONSrtTrack transY;

  DEFINE_SERIALIZABLE(JSONSrtMatrix, scaleX, scaleY, rot, transX, transY)

  static JSONSrtMatrix from(const SrtAnim::Mtx& mtx) {
    return {JSONSrtTrack::from(mtx.scaleX), JSONSrtTrack::from(mtx.scaleY),
            JSONSrtTrack::from(mtx.rot), JSONSrtTrack::from(mtx.transX),
            JSONSrtTrack::from(mtx.transY)};
  }

  SrtAnim::Mtx to() const {
    return {scaleX.to(), scaleY.to(), rot.to(), transX.to(), transY.to()};
  }
};

struct JSONSrtTarget {
  std::string materialName;
  bool indirect;
  int matrixIndex;

  DEFINE_SERIALIZABLE(JSONSrtTarget, materialName, indirect, matrixIndex)

  static JSONSrtTarget from(const SrtAnim::Target& target) {
    return {target.materialName, target.indirect, target.matrixIndex};
  }

  SrtAnim::Target to() const { return {materialName, indirect, matrixIndex}; }
};

struct JSONSrtTargetedMtx {
  JSONSrtTarget target;
  JSONSrtMatrix matrix;

  DEFINE_SERIALIZABLE(JSONSrtTargetedMtx, target, matrix)

  static JSONSrtTargetedMtx from(const SrtAnim::TargetedMtx& targetedMtx) {
    return {JSONSrtTarget::from(targetedMtx.target),
            JSONSrtMatrix::from(targetedMtx.matrix)};
  }

  SrtAnim::TargetedMtx to() const { return {target.to(), matrix.to()}; }
};

struct JSONSrtData {
  std::vector<JSONSrtTargetedMtx> matrices;
  std::string name;
  std::string sourcePath;
  u16 frameDuration;
  u32 xformModel;
  AnimationWrapMode wrapMode;

  DEFINE_SERIALIZABLE(JSONSrtData, matrices, name, sourcePath, frameDuration,
                      xformModel, wrapMode)

  static JSONSrtData from(const SrtAnim& anim) {
    JSONSrtData json;
    json.name = anim.name;
    json.sourcePath = anim.sourcePath;
    json.frameDuration = anim.frameDuration;
    json.xformModel = anim.xformModel;
    json.wrapMode = anim.wrapMode;
    for (const auto& targetedMtx : anim.matrices) {
      json.matrices.push_back(JSONSrtTargetedMtx::from(targetedMtx));
    }
    return json;
  }

  SrtAnim to() const {
    SrtAnim anim;
    anim.name = name;
    anim.sourcePath = sourcePath;
    anim.frameDuration = frameDuration;
    anim.xformModel = xformModel;
    anim.wrapMode = wrapMode;
    for (const auto& jsonTargetedMtx : matrices) {
      anim.matrices.push_back(jsonTargetedMtx.to());
    }
    return anim;
  }
};

struct ChrFramePacker {
  constexpr static size_t CHR_FRAME_SIZE = 8 * 3;
  static_assert(sizeof(librii::g3d::ChrFrame) == CHR_FRAME_SIZE);

  static std::vector<u8>
  pack(const std::vector<librii::g3d::ChrFrame>& frames) {
    return packEntries<librii::g3d::ChrFrame>(frames);
  }
  static std::vector<librii::g3d::ChrFrame> unpack(std::span<const u8> bytes) {
    return unpackEntries<librii::g3d::ChrFrame>(bytes);
  }
};

struct JSONChrTrack {
  ChrQuantization quant{};
  f32 scale{};
  f32 offset{};
  u32 framesDataBufferId{};
  u32 numKeyFrames{};

  DEFINE_SERIALIZABLE(JSONChrTrack, quant, scale, offset, framesDataBufferId,
                      numKeyFrames)

  static JSONChrTrack from(const ChrTrack& track, JsonWriteCtx& ctx) {
    JSONChrTrack jsonTrack;
    jsonTrack.quant = track.quant;
    jsonTrack.scale = track.scale;
    jsonTrack.offset = track.offset;
    jsonTrack.framesDataBufferId =
        ctx.save_buffer_with_move(ChrFramePacker::pack(track.frames));
    jsonTrack.numKeyFrames = track.frames.size();
    return jsonTrack;
  }

  Result<ChrTrack> to(std::span<const std::vector<u8>> buffers) const {
    ChrTrack track;
    track.quant = quant;
    track.scale = scale;
    track.offset = offset;
    if (framesDataBufferId >= buffers.size()) {
      return RSL_UNEXPECTED("Invalid buffer index");
    }
    track.frames = ChrFramePacker::unpack(buffers[framesDataBufferId]);
    return track;
  }
};

struct JSONChrNode {
  std::string name;
  u32 flags;
  std::vector<u32> tracks;

  DEFINE_SERIALIZABLE(JSONChrNode, name, flags, tracks)

  static JSONChrNode from(const ChrNode& node) {
    JSONChrNode jsonNode;
    jsonNode.name = node.name;
    jsonNode.flags = node.flags;
    jsonNode.tracks = node.tracks;
    return jsonNode;
  }

  ChrNode to() const {
    ChrNode node;
    node.name = name;
    node.flags = flags;
    node.tracks = tracks;
    return node;
  }
};

struct JSONChrData {
  std::vector<JSONChrNode> nodes;
  std::vector<JSONChrTrack> tracks;
  std::string name;
  std::string sourcePath;
  uint16_t frameDuration;
  uint32_t wrapMode;
  uint32_t scaleRule;

  DEFINE_SERIALIZABLE(JSONChrData, nodes, tracks, name, sourcePath,
                      frameDuration, wrapMode, scaleRule)

  static JSONChrData from(const ChrAnim& anim, JsonWriteCtx& ctx) {
    JSONChrData jsonAnim;
    jsonAnim.name = anim.name;
    jsonAnim.sourcePath = anim.sourcePath;
    jsonAnim.frameDuration = anim.frameDuration;
    jsonAnim.wrapMode = static_cast<u32>(anim.wrapMode);
    jsonAnim.scaleRule = anim.scaleRule;

    for (const auto& node : anim.nodes) {
      jsonAnim.nodes.push_back(JSONChrNode::from(node));
    }

    for (const auto& track : anim.tracks) {
      jsonAnim.tracks.push_back(JSONChrTrack::from(track, ctx));
    }

    return jsonAnim;
  }

  Result<ChrAnim> to(std::span<const std::vector<u8>> buffers) const {
    ChrAnim anim;
    anim.name = name;
    anim.sourcePath = sourcePath;
    anim.frameDuration = frameDuration;
    anim.wrapMode = static_cast<librii::g3d::AnimationWrapMode>(wrapMode);
    anim.scaleRule = scaleRule;

    for (const auto& jsonNode : nodes) {
      anim.nodes.push_back(jsonNode.to());
    }

    for (const auto& jsonTrack : tracks) {
      anim.tracks.push_back(TRY(jsonTrack.to(buffers)));
    }

    return anim;
  }
};

struct JSONClrKeyFrame {
  u32 data;

  DEFINE_SERIALIZABLE(JSONClrKeyFrame, data)

  static JSONClrKeyFrame from(const CLR0KeyFrame& keyFrame) {
    return {keyFrame.data};
  }

  CLR0KeyFrame to() const { return {data}; }
};

struct JSONClrTrack {
  std::vector<JSONClrKeyFrame> keyframes;

  DEFINE_SERIALIZABLE(JSONClrTrack, keyframes)

  static JSONClrTrack from(const CLR0Track& track) {
    JSONClrTrack jsonTrack;
    for (const auto& keyFrame : track.keyframes) {
      jsonTrack.keyframes.push_back(JSONClrKeyFrame::from(keyFrame));
    }
    return jsonTrack;
  }

  CLR0Track to() const {
    CLR0Track track;
    for (const auto& jsonKeyFrame : keyframes) {
      track.keyframes.push_back(jsonKeyFrame.to());
    }
    return track;
  }
};

struct JSONClrTarget {
  u32 notAnimatedMask;
  u32 data;

  DEFINE_SERIALIZABLE(JSONClrTarget, notAnimatedMask, data)

  static JSONClrTarget from(const ClrTarget& target) {
    return {target.notAnimatedMask, target.data};
  }

  ClrTarget to() const { return {notAnimatedMask, data}; }
};

struct JSONClrMaterial {
  std::string name;
  u32 flags;
  std::vector<JSONClrTarget> targets;

  DEFINE_SERIALIZABLE(JSONClrMaterial, name, flags, targets)

  static JSONClrMaterial from(const ClrMaterial& material) {
    JSONClrMaterial jsonMaterial;
    jsonMaterial.name = material.name;
    jsonMaterial.flags = material.flags;
    for (const auto& target : material.targets) {
      jsonMaterial.targets.push_back(JSONClrTarget::from(target));
    }
    return jsonMaterial;
  }

  ClrMaterial to() const {
    ClrMaterial material;
    material.name = name;
    material.flags = flags;
    for (const auto& jsonTarget : targets) {
      material.targets.push_back(jsonTarget.to());
    }
    return material;
  }
};

struct JSONClrAnim {
  std::vector<JSONClrMaterial> materials;
  std::vector<JSONClrTrack> tracks;
  std::string name;
  std::string sourcePath;
  u16 frameDuration;
  AnimationWrapMode wrapMode;

  DEFINE_SERIALIZABLE(JSONClrAnim, materials, tracks, name, sourcePath,
                      frameDuration, wrapMode)

  static JSONClrAnim from(const ClrAnim& anim) {
    JSONClrAnim json;
    json.name = anim.name;
    json.sourcePath = anim.sourcePath;
    json.frameDuration = anim.frameDuration;
    json.wrapMode = anim.wrapMode;
    for (const auto& material : anim.materials) {
      json.materials.push_back(JSONClrMaterial::from(material));
    }
    for (const auto& track : anim.tracks) {
      json.tracks.push_back(JSONClrTrack::from(track));
    }
    return json;
  }

  ClrAnim to() const {
    ClrAnim anim;
    anim.name = name;
    anim.sourcePath = sourcePath;
    anim.frameDuration = frameDuration;
    anim.wrapMode = wrapMode;
    for (const auto& jsonMaterial : materials) {
      anim.materials.push_back(jsonMaterial.to());
    }
    for (const auto& jsonTrack : tracks) {
      anim.tracks.push_back(jsonTrack.to());
    }
    return anim;
  }
};

struct JSONPatKeyFrame {
  float frame;
  u16 texture;
  u16 palette;

  DEFINE_SERIALIZABLE(JSONPatKeyFrame, frame, texture, palette)

  static JSONPatKeyFrame from(float frame, const PAT0KeyFrame& keyFrame) {
    return {frame, keyFrame.texture, keyFrame.palette};
  }

  PAT0KeyFrame to() const { return {texture, palette}; }
};

struct JSONPatTrack {
  std::vector<JSONPatKeyFrame> keyframes;
  u16 reserved;
  float progressPerFrame;

  DEFINE_SERIALIZABLE(JSONPatTrack, keyframes, reserved, progressPerFrame)

  static JSONPatTrack from(const PAT0Track& track) {
    JSONPatTrack jsonTrack;
    for (const auto& [frame, keyFrame] : track.keyframes) {
      jsonTrack.keyframes.push_back(JSONPatKeyFrame::from(frame, keyFrame));
    }
    jsonTrack.reserved = track.reserved;
    jsonTrack.progressPerFrame = track.progressPerFrame;
    return jsonTrack;
  }

  PAT0Track to() const {
    PAT0Track track;
    for (const auto& frame : keyframes) {
      track.keyframes[frame.frame] = frame.to();
    }
    track.reserved = reserved;
    track.progressPerFrame = progressPerFrame;
    return track;
  }
};

struct JSONPatSampler {
  std::string name;
  u32 flags;
  std::vector<u32> tracks;

  DEFINE_SERIALIZABLE(JSONPatSampler, name, flags, tracks)

  static JSONPatSampler from(const PatMaterial& material) {
    JSONPatSampler jsonSampler;
    jsonSampler.name = material.name;
    jsonSampler.flags = material.flags;
    for (const auto& track : material.samplers) {
      jsonSampler.tracks.push_back(track);
    }
    return jsonSampler;
  }

  PatMaterial to() const {
    PatMaterial material;
    material.name = name;
    material.flags = flags;
    for (const auto& jsonTrack : tracks) {
      material.samplers.push_back(jsonTrack);
    }
    return material;
  }
};

struct JSONPatAnim {
  std::vector<JSONPatSampler> samplers;
  std::vector<JSONPatTrack> tracks;
  std::vector<std::string> textureNames;
  std::vector<std::string> paletteNames;
  std::vector<u32> runtimeTextures;
  std::vector<u32> runtimePalettes;
  std::string name;
  std::string sourcePath;
  u16 frameDuration;
  AnimationWrapMode wrapMode;

  DEFINE_SERIALIZABLE(JSONPatAnim, samplers, tracks, textureNames, paletteNames,
                      runtimeTextures, runtimePalettes, name, sourcePath,
                      frameDuration, wrapMode)

  static JSONPatAnim from(const PatAnim& anim) {
    JSONPatAnim json;
    json.textureNames = anim.textureNames;
    json.paletteNames = anim.paletteNames;
    json.runtimeTextures = anim.runtimeTextures;
    json.runtimePalettes = anim.runtimePalettes;
    json.name = anim.name;
    json.sourcePath = anim.sourcePath;
    json.frameDuration = anim.frameDuration;
    json.wrapMode = anim.wrapMode;
    for (const auto& sampler : anim.materials) {
      json.samplers.push_back(JSONPatSampler::from(sampler));
    }
    for (const auto& track : anim.tracks) {
      json.tracks.push_back(JSONPatTrack::from(track));
    }
    return json;
  }

  PatAnim to() const {
    PatAnim anim;
    anim.textureNames = textureNames;
    anim.paletteNames = paletteNames;
    anim.runtimeTextures = runtimeTextures;
    anim.runtimePalettes = runtimePalettes;
    anim.name = name;
    anim.sourcePath = sourcePath;
    anim.frameDuration = frameDuration;
    anim.wrapMode = wrapMode;
    for (const auto& jsonSampler : samplers) {
      anim.materials.push_back(jsonSampler.to());
    }
    for (const auto& jsonTrack : tracks) {
      anim.tracks.push_back(jsonTrack.to());
    }
    return anim;
  }
};
struct JSONVisKeyFrame {
  u32 data;

  DEFINE_SERIALIZABLE(JSONVisKeyFrame, data)

  static JSONVisKeyFrame from(const VIS0KeyFrame& keyFrame) {
    return {keyFrame.data};
  }

  VIS0KeyFrame to() const { return {data}; }
};

struct JSONVisTrack {
  std::vector<JSONVisKeyFrame> keyframes;

  DEFINE_SERIALIZABLE(JSONVisTrack, keyframes)

  static JSONVisTrack from(const VIS0Track& track) {
    JSONVisTrack jsonTrack;
    for (const auto& keyFrame : track.keyframes) {
      jsonTrack.keyframes.push_back(JSONVisKeyFrame::from(keyFrame));
    }
    return jsonTrack;
  }

  VIS0Track to() const {
    VIS0Track track;
    for (const auto& jsonKeyFrame : keyframes) {
      track.keyframes.push_back(jsonKeyFrame.to());
    }
    return track;
  }
};

struct JSONVisBone {
  std::string name;
  u32 flags;
  std::optional<JSONVisTrack> target;

  DEFINE_SERIALIZABLE(JSONVisBone, name, flags, target)

  static JSONVisBone from(const VIS0Bone& bone) {
    JSONVisBone jsonBone;
    jsonBone.name = bone.name;
    jsonBone.flags = bone.flags;
    if (bone.target.has_value()) {
      jsonBone.target = JSONVisTrack::from(bone.target.value());
    }
    return jsonBone;
  }

  VIS0Bone to() const {
    VIS0Bone bone;
    bone.name = name;
    bone.flags = flags;
    if (target.has_value()) {
      bone.target = target->to();
    }
    return bone;
  }
};

struct JSONVisData {
  std::vector<JSONVisBone> bones;
  std::string name;
  std::string sourcePath;
  u16 frameDuration;
  AnimationWrapMode wrapMode;

  DEFINE_SERIALIZABLE(JSONVisData, bones, name, sourcePath, frameDuration,
                      wrapMode)

  static JSONVisData from(const BinaryVis& vis, JsonWriteCtx& ctx) {
    JSONVisData json;
    json.name = vis.name;
    json.sourcePath = vis.sourcePath;
    json.frameDuration = vis.frameDuration;
    json.wrapMode = vis.wrapMode;
    for (const auto& bone : vis.bones) {
      json.bones.push_back(JSONVisBone::from(bone));
    }
    return json;
  }

  BinaryVis to(std::span<const std::vector<u8>> buffers) const {
    BinaryVis vis;
    vis.name = name;
    vis.sourcePath = sourcePath;
    vis.frameDuration = frameDuration;
    vis.wrapMode = wrapMode;
    for (const auto& jsonBone : bones) {
      vis.bones.push_back(jsonBone.to());
    }
    return vis;
  }
};

struct JSONArchive {
  DEFINE_SERIALIZABLE(JSONArchive, models, textures, chrs, clrs, pats, srts,
                      viss)

  std::vector<JSONModel> models;
  std::vector<JSONTextureData> textures;
  std::vector<JSONChrData> chrs;
  std::vector<JSONClrAnim> clrs;
  std::vector<JSONPatAnim> pats;
  std::vector<JSONSrtData> srts;
  std::vector<JSONVisData> viss;

  static JSONArchive from(const librii::g3d::Archive& original,
                          JsonWriteCtx& c) {
    JSONArchive archive;

    for (const auto& model : original.models) {
      archive.models.emplace_back(JSONModel::from(model, c));
    }

    for (const auto& texture : original.textures) {
      archive.textures.emplace_back(JSONTextureData::from(texture, c));
    }

    for (const auto& chr : original.chrs) {
      archive.chrs.emplace_back(JSONChrData::from(chr, c));
    }

    for (const auto& clr : original.clrs) {
      archive.clrs.emplace_back(JSONClrAnim::from(clr));
    }

    for (const auto& pat : original.pats) {
      archive.pats.emplace_back(JSONPatAnim::from(pat));
    }

    for (const auto& srt : original.srts) {
      archive.srts.emplace_back(JSONSrtData::from(srt));
    }

    for (const auto& vis : original.viss) {
      archive.viss.emplace_back(JSONVisData::from(vis, c));
    }

    return archive;
  }

  Result<Archive> lift(std::span<const std::vector<u8>> buffers) const {
    Archive result;

    for (const auto& jsonModel : models) {
      result.models.push_back(TRY(jsonModel.lift(buffers)));
    }

    for (const auto& jsonTexture : textures) {
      result.textures.push_back(jsonTexture.to(buffers));
    }

    for (const auto& jsonChr : chrs) {
      result.chrs.push_back(TRY(jsonChr.to(buffers)));
    }

    for (const auto& jsonClr : clrs) {
      result.clrs.push_back(jsonClr.to());
    }

    for (const auto& jsonPat : pats) {
      result.pats.push_back(jsonPat.to());
    }

    for (const auto& jsonSrt : srts) {
      result.srts.push_back(jsonSrt.to());
    }

    for (const auto& jsonVis : viss) {
      result.viss.push_back(jsonVis.to(buffers));
    }

    return result;
  }
};

std::string ArcToJSON(const g3d::Archive& model, JsonWriteCtx& c) {
  auto mat = JSONArchive::from(model, c);
  return JS::serializeStruct(
      mat, JS::SerializerOptions(JS::SerializerOptions::Pretty));
}
Result<g3d::Archive> JSONToArc(std::string_view json,
                               std::span<const std::vector<u8>> buffers) {
  JS::ParseContext context(json.data(), json.size());
  JSONArchive mdl;
  auto err = context.parseTo(mdl);
  if (err != JS::Error::NoError) {
    auto n = magic_enum::enum_name(err);
    return RSL_UNEXPECTED(std::format("JSONToArc failed: Parse error {} ({})",
                                      n, context.makeErrorString()));
  }
  return mdl.lift(buffers);
}

} // namespace librii::g3d
