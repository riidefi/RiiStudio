#include <glm/vec3.hpp>
#include <iostream>
#include <string>
#include <vector>

#include <rsl/Types.hpp>

#include <librii/g3d/data/BoneData.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <librii/g3d/data/ModelData.hpp>
#include <librii/g3d/data/PolygonData.hpp>
#include <librii/g3d/data/VertexData.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#endif

#include <magic_enum/magic_enum.hpp>

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <rsl/EnumCast.hpp>

#include <vendor/json_struct.h>

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
    TypeHandler<std::vector<f32>>::to(t, context);
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

} // namespace JS

JS_OBJ_EXT(glm::vec3, x, y, z);
JS_OBJ_EXT(glm::vec4, x, y, z, w);
JS_OBJ_EXT(librii::gx::Color, r, g, b, a);
JS_OBJ_EXT(librii::gx::ColorS10, r, g, b, a);
namespace JS {
template <typename T, size_t N> struct TypeHandler<std::array<T, N>> {
  static inline Error to(auto& to_type, ParseContext& context) {
    return Error::NoError;
  }

  static inline void from(auto& vec, Token& token, Serializer& serializer) {
    token.value_type = Type::ArrayStart;
    token.value = DataRef("[");
    serializer.write(token);

    token.name = DataRef("");

    for (auto& index : vec) {
      TypeHandler<T>::from(index, token, serializer);
    }

    token.name = DataRef("");

    token.value_type = Type::ArrayEnd;
    token.value = DataRef("]");
    serializer.write(token);
  }
};
template <typename T>
concept IsEnum = std::is_enum_v<T>;
template <IsEnum T> struct TypeHandler<T> {
  static inline Error to(auto& to_type, ParseContext& context) {
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
                      evpmtx_mode)
  std::string scaling_rule = "Maya";
  std::string texmtx_mode = "Maya";
  std::string source_location = "";
  std::string evpmtx_mode = "Normal";
  // rfl::Field<"min", glm::vec3> min = glm::vec3{0.0f, 0.0f, 0.0f};
  // rfl::Field<"max", glm::vec3> max = glm::vec3{0.0f, 0.0f, 0.0f};

  static JSONModelInfo from(const ModelInfo& x) {
    JSONModelInfo result;

    result.scaling_rule = magic_enum::enum_name(x.scalingRule);
    result.texmtx_mode = magic_enum::enum_name(x.texMtxMode);
    result.source_location = x.sourceLocation;
    result.evpmtx_mode = magic_enum::enum_name(x.evpMtxMode);

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

    // TODO: Using only S16 is wasteful / won't match
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
struct JSONIndirectStage {
  DEFINE_SERIALIZABLE(JSONIndirectStage, indStageSel, format, bias, matrix,
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

  static JSONIndirectStage from(const TevStage::IndirectStage& original) {
    JSONIndirectStage json;
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
  JSONIndirectStage indirectStage;

  static JSONTevStage from(const TevStage& original) {
    JSONTevStage json;
    json.rasOrder = original.rasOrder;
    json.texMap = original.texMap;
    json.texCoord = original.texCoord;
    json.rasSwap = original.rasSwap;
    json.texMapSwap = original.texMapSwap;
    json.colorStage = JSONColorStage::from(original.colorStage);
    json.alphaStage = JSONAlphaStage::from(original.alphaStage);
    json.indirectStage = JSONIndirectStage::from(original.indirectStage);
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
struct JSONG3dMaterialData {
  DEFINE_SERIALIZABLE(JSONG3dMaterialData, flag, id, lightSetIndex, fogIndex,
                      name, texMatrices, samplers, cullMode, chanData,
                      colorChanControls, texGens, tevKonstColors, tevColors,
                      earlyZComparison, mStages)

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
template <typename T>
std::vector<T> unpackEntries(const std::vector<u8>& buffer) {
  std::vector<T> entries(buffer.size() / sizeof(T));
  std::memcpy(entries.data(), buffer.data(), buffer.size());
  return entries;
}

struct JSONPositionBuffer {
  DEFINE_SERIALIZABLE(JSONPositionBuffer, name, id, q_type, q_divisor, q_stride,
                      q_comp, dataBufferId)

  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Generic */ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::Position */ u32 q_comp;
  u32 dataBufferId;

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
    return buffer;
  }
};

struct JSONNormalBuffer {
  DEFINE_SERIALIZABLE(JSONNormalBuffer, name, id, q_type, q_divisor, q_stride,
                      q_comp, dataBufferId)

  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Generic */ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::Normal */ u32 q_comp;
  u32 dataBufferId;

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
    return buffer;
  }
};

struct JSONColorBuffer {
  DEFINE_SERIALIZABLE(JSONColorBuffer, name, id, q_type, q_divisor, q_stride,
                      q_comp, dataBufferId)

  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Color*/ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::Color*/ u32 q_comp;
  u32 dataBufferId;

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
    return buffer;
  }
};

struct JSONTextureCoordinateBuffer {
  DEFINE_SERIALIZABLE(JSONTextureCoordinateBuffer, name, id, q_type, q_divisor,
                      q_stride, q_comp, dataBufferId)
  std::string name;
  u32 id;
  /* librii::gx::VertexBufferType::Generic */ u32 q_type;
  u8 q_divisor;
  u8 q_stride;
  /* VCC::TextureCoordinate */ u32 q_comp;
  u32 dataBufferId;

  static JSONTextureCoordinateBuffer
  from(const TextureCoordinateBuffer& original, JsonWriteCtx& ctx) {
    JSONTextureCoordinateBuffer json;
    json.name = original.mName;
    json.id = original.mId;
    json.q_type = static_cast<u32>(original.mQuantize.mType.generic);
    json.q_divisor = original.mQuantize.divisor;
    json.q_stride = original.mQuantize.stride;
    json.q_comp = static_cast<u32>(original.mQuantize.mComp.normal);
    json.dataBufferId =
        ctx.save_buffer_with_move(packEntries(original.mEntries));
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

std::string ModelToJSON(const Model& model) {
  JsonWriteCtx ctx;
  JSONModel mdl = JSONModel::from(model, ctx);
  return JS::serializeStruct(mdl);
}
std::string ModelToJSON(const Model& model, JsonWriteCtx& ctx) {
  JSONModel mdl = JSONModel::from(model, ctx);
  return JS::serializeStruct(mdl);
}
Result<Model> JSONToModel(std::string_view json) {
  JS::ParseContext context(json.data(), json.size());
  JSONModel mdl;
  auto err = context.parseTo(mdl);
  if (err != JS::Error::NoError) {
    auto n = magic_enum::enum_name(err);
    return std::unexpected(
        std::format("JSONToModel failed: Parse error {}", n));
  }
  std::vector<std::vector<u8>> buffers;
  return mdl.lift(buffers);
}

std::string MatToJSON(const G3dMaterialData& model) {
  auto mat = JSONG3dMaterialData::from(model);
  return JS::serializeStruct(mat);
}
Result<G3dMaterialData> JSONToMat(std::string_view json) {
  JS::ParseContext context(json.data(), json.size());
  JSONG3dMaterialData mdl;
  auto err = context.parseTo(mdl);
  if (err != JS::Error::NoError) {
    auto n = magic_enum::enum_name(err);
    return std::unexpected(
        std::format("JSONToModel failed: Parse error {}", n));
  }
  return mdl;
}

std::string PolyToJSON(const g3d::PolygonData& model, JsonWriteCtx& c) {
  auto mat = JSONPolygonData::from(model, c);
  return JS::serializeStruct(mat);
}
Result<g3d::PolygonData> JSONToPoly(std::string_view json,
                                    std::span<const std::vector<u8>> buffers) {
  JS::ParseContext context(json.data(), json.size());
  JSONPolygonData mdl;
  auto err = context.parseTo(mdl);
  if (err != JS::Error::NoError) {
    auto n = magic_enum::enum_name(err);
    return std::unexpected(
        std::format("JSONToModel failed: Parse error {}", n));
  }
  return mdl.to(buffers);
}
std::string BoneToJSON(const g3d::BoneData& model) {
  auto mat = JSONBoneData::from(model);
  return JS::serializeStruct(mat);
}
Result<g3d::BoneData> JSONToBone(std::string_view json) {
  JS::ParseContext context(json.data(), json.size());
  JSONBoneData mdl;
  auto err = context.parseTo(mdl);
  if (err != JS::Error::NoError) {
    auto n = magic_enum::enum_name(err);
    return std::unexpected(
        std::format("JSONToModel failed: Parse error {}", n));
  }
  return mdl;
}

std::string MtxToJSON(const g3d::DrawMatrix& model) {
  auto mat = JSONDrawMatrix::from(model);
  return JS::serializeStruct(mat);
}
Result<g3d::DrawMatrix> JSONToMtx(std::string_view json) {
  JS::ParseContext context(json.data(), json.size());
  JSONDrawMatrix mdl;
  auto err = context.parseTo(mdl);
  if (err != JS::Error::NoError) {
    auto n = magic_enum::enum_name(err);
    return std::unexpected(
        std::format("JSONToModel failed: Parse error {}", n));
  }
  return mdl;
}

std::string InfoToJSON(const g3d::ModelInfo& model) {
  auto mat = JSONModelInfo::from(model);
  return JS::serializeStruct(mat);
}
Result<g3d::ModelInfo> JSONToInfo(std::string_view json) {
  JS::ParseContext context(json.data(), json.size());
  JSONModelInfo mdl;
  auto err = context.parseTo(mdl);
  if (err != JS::Error::NoError) {
    auto n = magic_enum::enum_name(err);
    return std::unexpected(
        std::format("JSONToModel failed: Parse error {}", n));
  }
  return mdl.to();
}

} // namespace librii::g3d
