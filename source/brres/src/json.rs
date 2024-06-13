use serde::Deserialize;
use serde::Serialize;

use std::fs::File;
use std::io::BufReader;

use crate::enums::*;

// This file has JSON schema

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONMatrixWeight {
    pub bone_id: u32,
    pub weight: f32,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONDrawMatrix {
    pub weights: Vec<JSONMatrixWeight>,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONModelInfo {
    pub scaling_rule: String,
    pub texmtx_mode: String,
    pub source_location: String,
    pub evpmtx_mode: String,
    pub min: [f32; 3],
    pub max: [f32; 3],
}

impl Default for JSONModelInfo {
    fn default() -> Self {
        JSONModelInfo {
            scaling_rule: "Maya".to_string(),
            texmtx_mode: "Maya".to_string(),
            source_location: "".to_string(),
            evpmtx_mode: "Normal".to_string(),
            min: [0.0, 0.0, 0.0],
            max: [0.0, 0.0, 0.0],
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct JsonMesh {
    pub clr_buffer: Vec<String>,
    pub current_matrix: i32,
    pub mprims: Vec<JsonPrimitive>,
    pub name: String,
    pub nrm_buffer: String,
    pub pos_buffer: String,
    pub uv_buffer: Vec<String>,
    pub vcd: i32,
    pub visible: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct JsonPrimitive {
    pub matrices: Vec<i32>,
    pub num_prims: i32,
    pub vertexDataBufferId: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct JsonModel {
    pub name: String,
    pub info: JSONModelInfo,
    pub bones: Vec<JSONBoneData>,
    pub materials: Vec<JSONMaterial>,
    pub matrices: Vec<JSONDrawMatrix>,
    pub meshes: Vec<JsonMesh>,
    pub positions: Vec<JsonBufferData<[f32; 3]>>,
    pub normals: Vec<JsonBufferData<[f32; 3]>>,
    pub colors: Vec<JsonBufferData<[u8; 4]>>,
    pub texcoords: Vec<JsonBufferData<[f32; 2]>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct JsonBufferData<T> {
    pub dataBufferId: i32,
    pub id: i32,
    pub name: String,
    pub q_comp: i32,
    pub q_divisor: i32,
    pub q_stride: i32,
    pub q_type: i32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cached_min: Option<T>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cached_max: Option<T>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct JsonTexture {
    pub dataBufferId: i32,
    pub format: u32,
    pub height: u32,
    pub maxLod: f32,
    pub minLod: f32,
    pub name: String,
    pub number_of_images: u32,
    pub sourcePath: String,
    pub width: u32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct JsonArchive {
    pub chrs: Vec<JSONChrData>,
    pub clrs: Vec<serde_json::Value>,
    pub models: Vec<JsonModel>,
    pub pats: Vec<JSONPatAnim>,
    pub srts: Vec<JSONSrtData>,
    pub textures: Vec<JsonTexture>,
    pub viss: Vec<serde_json::Value>,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONDrawCall {
    pub material: u32,
    pub poly: u32,
    pub prio: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONBoneData {
    pub name: String,
    pub ssc: bool,
    pub visible: bool,
    pub billboard_type: u32,
    pub scale: [f32; 3],
    pub rotate: [f32; 3],
    pub translate: [f32; 3],
    pub volume_min: [f32; 3],
    pub volume_max: [f32; 3],
    pub parent: i32,
    pub children: Vec<i32>,
    pub display_matrix: bool,
    pub draw_calls: Vec<JSONDrawCall>,
    pub force_display_matrix: bool,
    pub omit_from_node_mix: bool,
}

impl Default for JSONBoneData {
    fn default() -> Self {
        JSONBoneData {
            name: "Untitled Bone".to_string(),
            ssc: false,
            visible: true,
            billboard_type: 0,
            scale: [1.0, 1.0, 1.0],
            rotate: [0.0, 0.0, 0.0],
            translate: [0.0, 0.0, 0.0],
            volume_min: [0.0, 0.0, 0.0],
            volume_max: [0.0, 0.0, 0.0],
            parent: -1,
            children: Vec::new(),
            display_matrix: true,
            draw_calls: Vec::new(),
            force_display_matrix: false,
            omit_from_node_mix: false,
        }
    }
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone, Copy)]
pub enum ChrQuantization {
    Track32,
    Track48,
    Track96,
    BakedTrack8,
    BakedTrack16,
    BakedTrack32,
    Const,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct JSONChrTrack {
    pub quant: ChrQuantization,
    pub scale: f32,
    pub offset: f32,
    pub step: f32,
    pub framesDataBufferId: u32,
    pub numKeyFrames: u32,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct JSONChrNode {
    pub name: String,
    pub flags: u32,
    pub tracks: Vec<u32>,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct JSONChrData {
    pub nodes: Vec<JSONChrNode>,
    pub tracks: Vec<JSONChrTrack>,
    pub name: String,
    pub sourcePath: String,
    pub frameDuration: u16,
    pub wrapMode: u32,
    pub scaleRule: u32,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct ColorS10 {
    pub r: i16,
    pub g: i16,
    pub b: i16,
    pub a: i16,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONColorStage {
    pub constantSelection: TevKColorSel,
    pub a: TevColorArg,
    pub b: TevColorArg,
    pub c: TevColorArg,
    pub d: TevColorArg,
    pub formula: TevColorOp,
    pub bias: TevBias,
    pub scale: TevScale,
    pub clamp: bool,
    pub out: TevReg,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONAlphaStage {
    pub a: TevAlphaArg,
    pub b: TevAlphaArg,
    pub c: TevAlphaArg,
    pub d: TevAlphaArg,
    pub formula: TevAlphaOp,
    pub constantSelection: TevKAlphaSel,
    pub bias: TevBias,
    pub scale: TevScale,
    pub clamp: bool,
    pub out: TevReg,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONIndirectStageTev {
    pub indStageSel: u8,
    pub format: IndTexFormat,
    pub bias: IndTexBiasSel,
    pub matrix: IndTexMtxID,
    pub wrapU: IndTexWrap,
    pub wrapV: IndTexWrap,
    pub addPrev: bool,
    pub utcLod: bool,
    pub alpha: IndTexAlphaSel,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONTevStage {
    pub rasOrder: ColorSelChanApi,
    pub texMap: u8,
    pub texCoord: u8,
    pub rasSwap: u8,
    pub texMapSwap: u8,
    pub colorStage: JSONColorStage,
    pub alphaStage: JSONAlphaStage,
    pub indirectStage: JSONIndirectStageTev,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONTexMatrix {
    pub projection: TexGenType,
    pub scale: (f32, f32),
    pub rotate: f32,
    pub translate: (f32, f32),
    pub effectMatrix: [f32; 16],
    pub transformModel: CommonTransformModel,
    pub method: CommonMappingMethod,
    pub option: CommonMappingOption,
    pub camIdx: i8,
    pub lightIdx: i8,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONSamplerData {
    pub mTexture: String,
    pub mPalette: String,
    pub mWrapU: TextureWrapMode,
    pub mWrapV: TextureWrapMode,
    pub bEdgeLod: bool,
    pub bBiasClamp: bool,
    pub mMaxAniso: AnisotropyLevel,
    pub mMinFilter: TextureFilter,
    pub mMagFilter: TextureFilter,
    pub mLodBias: f32,
    pub btiId: u16,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONChannelControl {
    pub enabled: bool,
    pub Ambient: ColorSource,
    pub Material: ColorSource,
    pub lightMask: LightID,
    pub diffuseFn: DiffuseFunction,
    pub attenuationFn: AttenuationFunction,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONChannelData {
    pub matColor: Color,
    pub ambColor: Color,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONTexCoordGen {
    pub func: TexGenType,
    pub sourceParam: TexGenSrc,
    pub matrix: TexMatrix,
    pub normalize: bool,
    pub postMatrix: PostTexMatrix,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONMaterial {
    pub flag: u32,
    pub id: u32,
    pub lightSetIndex: i8,
    pub fogIndex: i8,
    pub name: String,
    pub texMatrices: Vec<JSONTexMatrix>, // MAX 10
    pub samplers: Vec<JSONSamplerData>,  // MAX 8
    pub cullMode: CullMode,
    pub chanData: Vec<JSONChannelData>,             // Max 2
    pub colorChanControls: Vec<JSONChannelControl>, // Max 4
    pub texGens: Vec<JSONTexCoordGen>,              // Max 8
    pub tevKonstColors: [Color; 4],
    pub tevColors: [ColorS10; 4],
    pub earlyZComparison: bool,
    pub zMode: JSONZMode,
    pub alphaCompare: JSONAlphaComparison,
    pub blendMode: JSONBlendMode,
    pub dstAlpha: JSONDstAlpha,
    pub xlu: bool,
    pub indirectStages: Vec<JSONIndirectStage>, // Max 4
    pub mIndMatrices: Vec<JSONIndirectMatrix>,  // Max 3
    pub mSwapTable: JSONSwapTable,
    pub mStages: Vec<JSONTevStage>, // Max 16
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONSwapTableEntry {
    pub r: ColorComponent,
    pub g: ColorComponent,
    pub b: ColorComponent,
    pub a: ColorComponent,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONSwapTable {
    pub entries: [JSONSwapTableEntry; 4],
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONIndOrder {
    pub refMap: u8,
    pub refCoord: u8,
}
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONIndirectTextureScalePair {
    pub U: IndirectTextureScalePairSelection,
    pub V: IndirectTextureScalePairSelection,
}
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONIndirectStage {
    pub scale: JSONIndirectTextureScalePair,
    pub order: JSONIndOrder,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONIndirectMatrix {
    pub scale: (f32, f32),
    pub rotate: f32,
    pub trans: (f32, f32),
    pub quant: i32,
    pub method: IndirectMatrixMethod,
    pub refLight: i8,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONZMode {
    pub compare: bool,
    pub function: Comparison,
    pub update: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONAlphaComparison {
    pub compLeft: Comparison,
    pub refLeft: u8,
    pub op: AlphaOp,
    pub compRight: Comparison,
    pub refRight: u8,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONBlendMode {
    #[serde(rename = "type")]
    pub type_: BlendModeType,
    pub source: BlendModeFactor,
    pub dest: BlendModeFactor,
    pub logic: LogicOp,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub struct JSONDstAlpha {
    pub enabled: bool,
    pub alpha: u8,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONSrtKeyFrame {
    pub frame: f32,
    pub value: f32,
    pub tangent: f32,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONSrtTrack {
    pub keyframes: Vec<JSONSrtKeyFrame>,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONSrtMatrix {
    pub scaleX: JSONSrtTrack,
    pub scaleY: JSONSrtTrack,
    pub rot: JSONSrtTrack,
    pub transX: JSONSrtTrack,
    pub transY: JSONSrtTrack,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONSrtTarget {
    pub materialName: String,
    pub indirect: bool,
    pub matrixIndex: i32,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONSrtTargetedMtx {
    pub target: JSONSrtTarget,
    pub matrix: JSONSrtMatrix,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONSrtData {
    pub matrices: Vec<JSONSrtTargetedMtx>,
    pub name: String,
    pub sourcePath: String,
    pub frameDuration: u16,
    pub xformModel: u32,
    pub wrapMode: AnimationWrapMode,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONPatKeyFrame {
    pub frame: f32,
    pub texture: u16,
    pub palette: u16,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONPatTrack {
    pub keyframes: Vec<JSONPatKeyFrame>,
    pub reserved: u16,
    pub progressPerFrame: f32,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONPatSampler {
    pub name: String,
    pub flags: u32,
    pub tracks: Vec<u32>,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct JSONPatAnim {
    pub samplers: Vec<JSONPatSampler>,
    pub tracks: Vec<JSONPatTrack>,
    pub textureNames: Vec<String>,
    pub paletteNames: Vec<String>,
    pub runtimeTextures: Vec<u32>,
    pub runtimePalettes: Vec<u32>,
    pub name: String,
    pub sourcePath: String,
    pub frameDuration: u16,
    pub wrapMode: AnimationWrapMode,
}

pub fn read_json(file_path: &str) -> anyhow::Result<JsonArchive> {
    let file = File::open(file_path)?;
    let reader = BufReader::new(file);
    let root: JsonArchive = serde_json::from_reader(reader)?;
    Ok(root)
}

#[cfg(test)]
mod tests3 {
    use super::*;
    #[test]
    fn dummy_json() {
        let file_path = "dummy.json";
        match read_json(file_path) {
            Ok(root) => println!("{:#?}", root),
            Err(e) => panic!("Error reading JSON file: {}", e),
        }
    }
}
