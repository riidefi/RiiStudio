#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevKColorSel {
    const_8_8,
    const_7_8,
    const_6_8,
    const_5_8,
    const_4_8,
    const_3_8,
    const_2_8,
    const_1_8,
    k0 = 12,
    k1,
    k2,
    k3,
    k0_r,
    k1_r,
    k2_r,
    k3_r,
    k0_g,
    k1_g,
    k2_g,
    k3_g,
    k0_b,
    k1_b,
    k2_b,
    k3_b,
    k0_a,
    k1_a,
    k2_a,
    k3_a,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevColorArg {
    cprev,
    aprev,
    c0,
    a0,
    c1,
    a1,
    c2,
    a2,
    texc,
    texa,
    rasc,
    rasa,
    one,
    half,
    konst,
    zero,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevColorOp {
    add,
    subtract,
    comp_r8_gt = 8,
    comp_r8_eq,
    comp_gr16_gt,
    comp_gr16_eq,
    comp_bgr24_gt,
    comp_bgr24_eq,
    comp_rgb8_gt,
    comp_rgb8_eq,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevBias {
    zero,
    add_half,
    sub_half,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevScale {
    scale_1,
    scale_2,
    scale_4,
    divide_2,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevReg {
    prev = 0,
    reg0,
    reg1,
    reg2,
    // reg3 = 0, // prev
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevAlphaArg {
    aprev,
    a0,
    a1,
    a2,
    texa,
    rasa,
    konst,
    zero,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevAlphaOp {
    add,
    subtract,
    comp_r8_gt = 8,
    comp_r8_eq,
    comp_gr16_gt,
    comp_gr16_eq,
    comp_bgr24_gt,
    comp_bgr24_eq,
    comp_a8_gt,
    comp_a8_eq,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TevKAlphaSel {
    const_8_8,
    const_7_8,
    const_6_8,
    const_5_8,
    const_4_8,
    const_3_8,
    const_2_8,
    const_1_8,
    k0 = 12,
    k1,
    k2,
    k3,
    k0_r = 16,
    k1_r,
    k2_r,
    k3_r,
    k0_g,
    k1_g,
    k2_g,
    k3_g,
    k0_b,
    k1_b,
    k2_b,
    k3_b,
    k0_a,
    k1_a,
    k2_a,
    k3_a,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum IndTexFormat {
    _8bit,
    _5bit,
    _4bit,
    _3bit,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum IndTexBiasSel {
    none,
    s,
    t,
    st,
    u,
    su,
    tu,
    stu,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TexGenType {
    Matrix3x4,
    Matrix2x4,
    Bump0,
    Bump1,
    Bump2,
    Bump3,
    Bump4,
    Bump5,
    Bump6,
    Bump7,
    SRTG,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum CommonTransformModel {
    Default,
    Maya,
    Max,
    XSI,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum CommonMappingMethod {
    Standard,
    EnvironmentMapping,
    ViewProjectionMapping,
    ProjectionMapping,
    // ManualProjectionMapping = ProjectionMapping,
    EnvironmentLightMapping,
    EnvironmentSpecularMapping,
    ManualEnvironmentMapping,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum CommonMappingOption {
    NoSelection,
    DontRemapTextureSpace,
    KeepTranslation,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TextureWrapMode {
    Clamp,
    Repeat,
    Mirror,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum AnisotropyLevel {
    x1,
    x2,
    x4,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TextureFilter {
    Near,
    Linear,
    near_mip_near,
    lin_mip_near,
    near_mip_lin,
    lin_mip_lin,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum ColorSource {
    Register,
    Vertex,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum LightID {
    None = 0,
    Light0 = 0x001,
    Light1 = 0x002,
    Light2 = 0x004,
    Light3 = 0x008,
    Light4 = 0x010,
    Light5 = 0x020,
    Light6 = 0x040,
    Light7 = 0x080,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum DiffuseFunction {
    None,
    Sign,
    Clamp,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum AttenuationFunction {
    Specular,
    Spotlight,
    None,
    None2,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TexGenSrc {
    Position,
    Normal,
    Binormal,
    Tangent,
    UV0,
    UV1,
    UV2,
    UV3,
    UV4,
    UV5,
    UV6,
    UV7,
    BumpUV0,
    BumpUV1,
    BumpUV2,
    BumpUV3,
    BumpUV4,
    BumpUV5,
    BumpUV6,
    Color0,
    Color1,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum TexMatrix {
    Identity = 60,
    TexMatrix0 = 30,
    TexMatrix1 = 33,
    TexMatrix2 = 36,
    TexMatrix3 = 39,
    TexMatrix4 = 42,
    TexMatrix5 = 45,
    TexMatrix6 = 48,
    TexMatrix7 = 51,
    TexMatrix8 = 54,
    TexMatrix9 = 57,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum CullMode {
    None,
    Front,
    Back,
    All,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum IndTexAlphaSel {
    off,
    s,
    t,
    u,
}
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum IndTexMtxID {
    off,
    _0,
    _1,
    _2,

    s0 = 5,
    s1,
    s2,

    t0 = 9,
    t1,
    t2,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum IndTexWrap {
    off,
    _256,
    _128,
    _64,
    _32,
    _16,
    _0,
}
use serde::Deserialize;
use serde::Serialize;

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum ColorSelChanApi {
    color0,
    color1,
    alpha0,
    alpha1,
    color0a0,
    color1a1,
    zero,

    ind_alpha,
    normalized_ind_alpha,
    null = 0xFF,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum PostTexMatrix {
    Matrix0 = 64,
    Matrix1 = 67,
    Matrix2 = 70,
    Matrix3 = 73,
    Matrix4 = 76,
    Matrix5 = 79,
    Matrix6 = 82,
    Matrix7 = 85,
    Matrix8 = 88,
    Matrix9 = 91,
    Matrix10 = 94,
    Matrix11 = 97,
    Matrix12 = 100,
    Matrix13 = 103,
    Matrix14 = 106,
    Matrix15 = 109,
    Matrix16 = 112,
    Matrix17 = 115,
    Matrix18 = 118,
    Matrix19 = 121,
    Identity = 125,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum IndirectTextureScalePairSelection {
    x_1,
    x_2,
    x_4,
    x_8,
    x_16,
    x_32,
    x_64,
    x_128,
    x_256,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum Comparison {
    NEVER,
    LESS,
    EQUAL,
    LEQUAL,
    GREATER,
    NEQUAL,
    GEQUAL,
    ALWAYS,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum AlphaOp {
    _and,
    _or,
    _xor,
    _xnor,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum BlendModeType {
    none,
    blend,
    logic,
    subtract,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum BlendModeFactor {
    zero,
    one,
    src_c,
    inv_src_c,
    src_a,
    inv_src_a,
    dst_a,
    inv_dst_a,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum LogicOp {
    _clear,
    _and,
    _rev_and,
    _copy,
    _inv_and,
    _no_op,
    _xor,
    _or,
    _nor,
    _equiv,
    _inv,
    _revor,
    _inv_copy,
    _inv_or,
    _nand,
    _set,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum ColorComponent {
    r,
    g,
    b,
    a,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum IndirectMatrixMethod {
    Warp,
    NormalMap,
    NormalMapSpec,
    Fur,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub enum AnimationWrapMode {
    Clamp,  // One-shot
    Repeat, // Loop
}
