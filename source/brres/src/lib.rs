use serde::Deserialize;
use serde::Serialize;
use serde_json::Result;
use std::fs::File;
use std::io::BufReader;
use std::vec::Vec;

mod buffers;
mod enums;
mod ffi;

use enums::*;

#[derive(Deserialize, Debug)]
struct JsonMesh {
    clr_buffer: Vec<String>,
    current_matrix: i32,
    mprims: Vec<JsonPrimitive>,
    name: String,
    nrm_buffer: String,
    pos_buffer: String,
    uv_buffer: Vec<String>,
    vcd: i32,
    visible: bool,
}

#[derive(Deserialize, Debug)]
struct JsonPrimitive {
    matrices: Vec<i32>,
    num_prims: i32,
    vertexDataBufferId: i32,
}

#[derive(Deserialize, Debug)]
struct JsonModel {
    bones: Vec<Option<serde_json::Value>>,
    colors: Vec<serde_json::Value>,
    materials: Vec<JSONMaterial>,
    matrices: Vec<Option<serde_json::Value>>,
    meshes: Vec<JsonMesh>,
    name: String,
    normals: Vec<JsonBufferData>,
    positions: Vec<JsonBufferData>,
    texcoords: Vec<JsonBufferData>,
}

#[derive(Deserialize, Debug)]
struct JsonBufferData {
    dataBufferId: i32,
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
}

#[derive(Deserialize, Debug)]
struct JsonTexture {
    dataBufferId: i32,
    format: u32,
    height: u32,
    maxLod: f32,
    minLod: f32,
    name: String,
    number_of_images: u32,
    sourcePath: String,
    width: u32,
}

#[derive(Deserialize, Debug)]
struct JsonArchive {
    chrs: Vec<serde_json::Value>,
    clrs: Vec<serde_json::Value>,
    models: Vec<JsonModel>,
    pats: Vec<serde_json::Value>,
    srts: Vec<serde_json::Value>,
    textures: Vec<JsonTexture>,
    viss: Vec<serde_json::Value>,
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

#[derive(Debug)]
pub struct Archive {
    pub models: Vec<Model>,
    pub textures: Vec<Texture>,

    // Animation data
    /// Bone (character motion) animation
    pub chrs: Vec<serde_json::Value>,

    /// Texture matrix ("SRT" a.k.a. "Scale, Rotate, Translate") animation
    pub srts: Vec<serde_json::Value>,

    /// Texture data (pattern) animation (e.g. like a GIF)
    pub pats: Vec<serde_json::Value>,

    /// GPU uniform (color) animation
    pub clrs: Vec<serde_json::Value>,

    /// Bone visibility animation
    pub viss: Vec<serde_json::Value>,
}

impl Archive {
    fn from_json(archive: JsonArchive, buffers: Vec<Vec<u8>>) -> Self {
        Self {
            chrs: archive.chrs,
            clrs: archive.clrs,
            models: archive
                .models
                .into_iter()
                .map(|m| Model::from_json(m, &buffers))
                .collect(),
            pats: archive.pats,
            srts: archive.srts,
            textures: archive
                .textures
                .into_iter()
                .map(|t| Texture::from_json(t, &buffers))
                .collect(),
            viss: archive.viss,
        }
    }

    fn get_model(&self, name: &str) -> Option<&Model> {
        self.models.iter().find(|model| model.name == name)
    }

    fn get_texture(&self, name: &str) -> Option<&Texture> {
        self.textures.iter().find(|texture| texture.name == name)
    }
}

#[derive(Debug)]
pub struct MatrixPrimitive {
    /// Indices into a `Model`'s `matrices` list.
    ///
    /// When skinning indices are used in the actual vertex data (`vertex_data_buffer` field), they are relative to this list.
    /// (e.g. if `MatrixPrimitive.matrices` was [1, 10, 23] then a vertex PNM index of `1` in a vertex would refer to `Model.matrices[10]`)
    pub matrices: Vec<i32>,

    /// Number of primitives (triangles, quads, tristrips, etc) contained in this draw call. Required for decoding the `vertex_data_buffer` field below.
    pub num_prims: i32,

    /**
    Stored as a giant buffer for convenience. To parse / edit it, the format is described below:

    Format:
    ```c
        struct MatrixPrimitiveBlob {
            Primitive prims[num_prims]; // field above
        };

        struct Primitive __packed__ {
            u8 primitive_type; // e.g. triangles, quads (GXPrimitiveType enum)
            u24 vertex_count;  // big endian
            IndexedVertex vertices[vertex_count];
        };

        struct IndexedVertex {
            u16 indices[26]; // e.g. indices[9] is position (GXAttr enum, GX_POSITION == 9)
        };
    ```

    Reference code to decode (untested):
    ```py
        cursor = 0
        for i in range(num_prims):
            prim_type = buf[cursor]
            prim_vtx_count = (buf[cursor + 1] << 16) | (buf[cursor + 2] << 8) | buf[cursor + 3]
            cursor += 4

            print(f"Primitive of type {prim_type}:")
            for j in range(prim_vtx_count):
                raw_vertex = buf[cursor:cursor+26*2]
                cursor += 26*2

                decoded_vertex = [(raw_vertex[p*2] << 8) | raw_vertex[p*2 + 1]) for p in range(26)]
                print(f"  -> Vertex: {decoded_vertex}")
    ```
    */
    pub vertex_data_buffer: Vec<u8>,
}

impl MatrixPrimitive {
    fn from_json(json_primitive: JsonPrimitive, buffers: &[Vec<u8>]) -> Self {
        let vertex_data_buffer = buffers
            .get(json_primitive.vertexDataBufferId as usize)
            .cloned()
            .unwrap_or_else(Vec::new);

        Self {
            matrices: json_primitive.matrices,
            num_prims: json_primitive.num_prims,
            vertex_data_buffer,
        }
    }
}

#[derive(Debug)]
pub struct Mesh {
    pub name: String,

    pub visible: bool,

    // String (for now) references to vertex data arrays in the Model
    pub clr_buffer: Vec<String>,
    pub nrm_buffer: String,
    pub pos_buffer: String,
    pub uv_buffer: Vec<String>,

    /// Bitfield of attributes (1 << n) for n in GXAttr enum
    pub vcd: i32,

    /// For unrigged meshes which matrix (in the Model matrices list) are we referencing? (usually 0, bound to the root bone)
    pub current_matrix: i32,

    /// Actual primitive data
    pub mprims: Vec<MatrixPrimitive>,
}

impl Mesh {
    fn from_json(json_mesh: JsonMesh, buffers: &[Vec<u8>]) -> Self {
        Self {
            clr_buffer: json_mesh.clr_buffer,
            current_matrix: json_mesh.current_matrix,
            mprims: json_mesh
                .mprims
                .into_iter()
                .map(|p| MatrixPrimitive::from_json(p, buffers))
                .collect(),
            name: json_mesh.name,
            nrm_buffer: json_mesh.nrm_buffer,
            pos_buffer: json_mesh.pos_buffer,
            uv_buffer: json_mesh.uv_buffer,
            vcd: json_mesh.vcd,
            visible: json_mesh.visible,
        }
    }
}

#[derive(Debug)]
pub struct Model {
    pub name: String,

    pub bones: Vec<Option<serde_json::Value>>,
    pub materials: Vec<JSONMaterial>,
    pub meshes: Vec<Mesh>,

    // Vertex buffers
    pub positions: Vec<JsonBufferData>,
    pub normals: Vec<JsonBufferData>,
    pub texcoords: Vec<JsonBufferData>,
    pub colors: Vec<serde_json::Value>,

    /// For skinning
    pub matrices: Vec<Option<serde_json::Value>>,
}

impl Model {
    fn from_json(model: JsonModel, buffers: &[Vec<u8>]) -> Self {
        Self {
            bones: model.bones,
            colors: model.colors,
            materials: model.materials,
            matrices: model.matrices,
            meshes: model
                .meshes
                .into_iter()
                .map(|m| Mesh::from_json(m, buffers))
                .collect(),
            name: model.name,
            normals: model.normals,
            positions: model.positions,
            texcoords: model.texcoords,
        }
    }
}

#[derive(Debug)]
pub struct Texture {
    pub name: String,

    /// Valid in range `[1, 1024]`. Hardware register is only 10 bits--you cannot exceed this limitation, even on emulator.
    pub width: u32,
    /// Valid in range `[1, 1024]`. Hardware register is only 10 bits--you cannot exceed this limitation, even on emulator.
    pub height: u32,

    /// GPU texture format to be directly loaded into VRAM. Most commonly CMPR a.k.a. BC1 a.k.a. DXT1 (`tex.format == 14`).
    ///
    /// Full enum (from gctex):
    ///
    /// ```rust
    /// pub enum TextureFormat {
    ///     I4 = 0,
    ///     I8,
    ///     IA4,
    ///     IA8,
    ///     RGB565,
    ///     RGB5A3,
    ///     RGBA8,
    ///
    ///     C4 = 0x8,
    ///     C8,
    ///     C14X2,
    ///     CMPR = 0xE,
    /// };
    /// ```
    /// With `gctex`, use `gctex::TextureFormat::from_u32(...)` on this field.
    pub format: u32,

    /// Set to `1` for no mipmaps. Valid in range `[1, 11]`. More tightly bounded above by `1 + floor(log2(min(width, height)))`.
    pub number_of_images: u32,

    /// Raw GPU texture data. Use [`gctex`](https://crates.io/crates/gctex) or similar to decode.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use gctex;
    ///
    /// let tex = brres::Texture {
    ///     name: "Example".to_string(),
    ///     width: 8,
    ///     height: 8,
    ///     format: 14,
    ///     number_of_images: 1,
    ///     data: vec![0xF7, 0xBE, 0x6B, 0x0D, 0x34, 0x0E, 0x0D, 0x02, 0xF7, 0x7D, 0x52, 0x4A, 0x20, 0x28, 0xD8, 0xE0,
    ///                0xFF, 0xDF, 0x08, 0x25, 0x00, 0xAA, 0xDE, 0xFC, 0xEF, 0x5C, 0x31, 0x07, 0x00, 0xF8, 0x5C, 0x9C],
    ///     min_lod: 0.0,
    ///     max_lod: 0.0,
    /// };
    ///
    /// let texture_format = gctex::TextureFormat::from_u32(tex.format).unwrap();
    /// let raw_rgba_buffer = gctex::decode(&tex.data, tex.width, tex.height, texture_format, &[0], 0);
    ///
    /// assert_eq!(raw_rgba_buffer,
    ///           [0xF7, 0xF7, 0xF7, 0xFF, 0x9F, 0x99, 0x9F, 0xFF, 0x6B, 0x61, 0x6B, 0xFF, 0xF7, 0xF7, 0xF7, 0xFF,
    ///            0xF7, 0xEF, 0xEF, 0xFF, 0xB9, 0xB0, 0xB4, 0xFF, 0xF7, 0xEF, 0xEF, 0xFF, 0xF7, 0xEF, 0xEF, 0xFF,
    ///            0xF7, 0xF7, 0xF7, 0xFF, 0xF7, 0xF7, 0xF7, 0xFF, 0x9F, 0x99, 0x9F, 0xFF, 0xC2, 0xBE, 0xC2, 0xFF,
    ///            0xF7, 0xEF, 0xEF, 0xFF, 0xB9, 0xB0, 0xB4, 0xFF, 0xB9, 0xB0, 0xB4, 0xFF, 0xF7, 0xEF, 0xEF, 0xFF,
    ///            0xF7, 0xF7, 0xF7, 0xFF, 0xF7, 0xF7, 0xF7, 0xFF, 0x9F, 0x99, 0x9F, 0xFF, 0x6B, 0x61, 0x6B, 0xFF,
    ///            0x8F, 0x87, 0x8C, 0xFF, 0x52, 0x49, 0x52, 0xFF, 0xB9, 0xB0, 0xB4, 0xFF, 0xF7, 0xEF, 0xEF, 0xFF,
    ///            0xF7, 0xF7, 0xF7, 0xFF, 0xF7, 0xF7, 0xF7, 0xFF, 0xF7, 0xF7, 0xF7, 0xFF, 0xC2, 0xBE, 0xC2, 0xFF,
    ///            0x8F, 0x87, 0x8C, 0xFF, 0xB9, 0xB0, 0xB4, 0xFF, 0xF7, 0xEF, 0xEF, 0xFF, 0xF7, 0xEF, 0xEF, 0xFF,
    ///            0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF,
    ///            0xEF, 0xEB, 0xE7, 0xFF, 0xEF, 0xEB, 0xE7, 0xFF, 0xEF, 0xEB, 0xE7, 0xFF, 0xEF, 0xEB, 0xE7, 0xFF,
    ///            0xA2, 0x9E, 0xAE, 0xFF, 0xA2, 0x9E, 0xAE, 0xFF, 0xA2, 0x9E, 0xAE, 0xFF, 0xA2, 0x9E, 0xAE, 0xFF,
    ///            0x78, 0x6C, 0x7A, 0xFF, 0x78, 0x6C, 0x7A, 0xFF, 0xA7, 0x9E, 0xA5, 0xFF, 0xEF, 0xEB, 0xE7, 0xFF,
    ///            0x64, 0x60, 0x79, 0xFF, 0x08, 0x04, 0x29, 0xFF, 0x64, 0x60, 0x79, 0xFF, 0xA2, 0x9E, 0xAE, 0xFF,
    ///            0x31, 0x20, 0x39, 0xFF, 0x31, 0x20, 0x39, 0xFF, 0x78, 0x6C, 0x7A, 0xFF, 0xEF, 0xEB, 0xE7, 0xFF,
    ///            0x64, 0x60, 0x79, 0xFF, 0x64, 0x60, 0x79, 0xFF, 0x64, 0x60, 0x79, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF,
    ///            0xA7, 0x9E, 0xA5, 0xFF, 0x31, 0x20, 0x39, 0xFF, 0x78, 0x6C, 0x7A, 0xFF, 0xEF, 0xEB, 0xE7, 0xFF]);
    /// ```
    pub data: Vec<u8>,

    /// Always recomputed on save (currently).
    /// Set to `0.0`.
    pub min_lod: f32,
    /// Always recomputed on save (currently).
    /// Set to `(tex.number_of_images - 1) as f32`.
    pub max_lod: f32,
}

impl Texture {
    fn from_json(texture: JsonTexture, buffers: &[Vec<u8>]) -> Self {
        let data = buffers
            .get(texture.dataBufferId as usize)
            .map(|buffer| buffer.clone())
            .unwrap_or_else(Vec::new);

        Self {
            data,
            format: texture.format,
            height: texture.height,
            max_lod: texture.maxLod,
            min_lod: texture.minLod,
            name: texture.name,
            number_of_images: texture.number_of_images,
            width: texture.width,
        }
    }
}

#[derive(Serialize, Deserialize, Debug)]
struct Color {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

#[derive(Serialize, Deserialize, Debug)]
struct ColorS10 {
    r: i16,
    g: i16,
    b: i16,
    a: i16,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONColorStage {
    constantSelection: TevKColorSel,
    a: TevColorArg,
    b: TevColorArg,
    c: TevColorArg,
    d: TevColorArg,
    formula: TevColorOp,
    bias: TevBias,
    scale: TevScale,
    clamp: bool,
    out: TevReg,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONAlphaStage {
    a: TevAlphaArg,
    b: TevAlphaArg,
    c: TevAlphaArg,
    d: TevAlphaArg,
    formula: TevAlphaOp,
    constantSelection: TevKAlphaSel,
    bias: TevBias,
    scale: TevScale,
    clamp: bool,
    out: TevReg,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONIndirectStage {
    indStageSel: u8,
    format: IndTexFormat,
    bias: IndTexBiasSel,
    matrix: IndTexMtxID,
    wrapU: IndTexWrap,
    wrapV: IndTexWrap,
    addPrev: bool,
    utcLod: bool,
    alpha: IndTexAlphaSel,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONTevStage {
    rasOrder: ColorSelChanApi,
    texMap: u8,
    texCoord: u8,
    rasSwap: u8,
    texMapSwap: u8,
    colorStage: JSONColorStage,
    alphaStage: JSONAlphaStage,
    indirectStage: JSONIndirectStage,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONTexMatrix {
    projection: TexGenType,
    scale: (f32, f32),
    rotate: f32,
    translate: (f32, f32),
    effectMatrix: [f32; 16],
    transformModel: CommonTransformModel,
    method: CommonMappingMethod,
    option: CommonMappingOption,
    camIdx: i8,
    lightIdx: i8,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONSamplerData {
    mTexture: String,
    mPalette: String,
    mWrapU: TextureWrapMode,
    mWrapV: TextureWrapMode,
    bEdgeLod: bool,
    bBiasClamp: bool,
    mMaxAniso: AnisotropyLevel,
    mMinFilter: TextureFilter,
    mMagFilter: TextureFilter,
    mLodBias: f32,
    btiId: u16,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONChannelControl {
    enabled: bool,
    Ambient: ColorSource,
    Material: ColorSource,
    lightMask: LightID,
    diffuseFn: DiffuseFunction,
    attenuationFn: AttenuationFunction,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONChannelData {
    matColor: Color,
    ambColor: Color,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONTexCoordGen {
    func: TexGenType,
    sourceParam: TexGenSrc,
    matrix: TexMatrix,
    normalize: bool,
    postMatrix: PostTexMatrix,
}

#[derive(Serialize, Deserialize, Debug)]
struct JSONMaterial {
    flag: u32,
    id: u32,
    lightSetIndex: i8,
    fogIndex: i8,
    name: String,
    texMatrices: Vec<JSONTexMatrix>, // MAX 10
    samplers: Vec<JSONSamplerData>,  // MAX 8
    cullMode: CullMode,
    chanData: Vec<JSONChannelData>,             // Max 2
    colorChanControls: Vec<JSONChannelControl>, // Max 4
    texGens: Vec<JSONTexCoordGen>,              // Max 8
    tevKonstColors: [Color; 4],
    tevColors: [ColorS10; 4],
    earlyZComparison: bool,
    mStages: Vec<JSONTevStage>, // Max 16
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_archive() {
        let file_path = "dummy.json";
        match read_json(file_path) {
            Ok(json_archive) => {
                // Assuming buffers are loaded here
                let buffers = vec![
                    vec![0, 1, 2, 3, 4], // Example buffer
                    vec![5, 6, 7, 8, 9],
                ];
                let archive = Archive::from_json(json_archive, buffers);
                // Add more tests to validate Archive methods
                assert!(archive.get_model("example").is_some());
                assert!(archive.get_texture("human_A").is_some());
            }
            Err(e) => panic!("Error reading JSON file: {}", e),
        }
    }
}

pub fn create_archive(json_str: &str, raw_buffer: &[u8]) -> anyhow::Result<Archive> {
    // Decode the JSON string to JsonArchive
    let json_archive: JsonArchive = serde_json::from_str(json_str)?;

    // Convert the raw buffer to Vec<Vec<u8>> using read_buffer
    let buffers = buffers::read_buffer(raw_buffer);

    // Create and return the Archive
    Ok(Archive::from_json(json_archive, buffers))
}

pub fn read_raw_brres(brres: &[u8]) -> anyhow::Result<Archive> {
    let tmp = ffi::CBrresWrapper::from_bytes(brres)?;

    create_archive(&tmp.json_metadata, &tmp.buffer_data)
}

#[cfg(test)]
mod tests4 {
    use super::*;
    use std::fs;

    #[test]
    fn test_read_raw_brres() {
        // Read the sea.brres file into a byte array
        let brres_data =
            fs::read("../../tests/samples/sea.brres").expect("Failed to read sea.brres file");

        // Call the read_raw_brres function
        match read_raw_brres(&brres_data) {
            Ok(archive) => {
                println!("{:#?}", archive.get_model("sea").unwrap().meshes[0]);
                println!("{:#?}", archive.get_model("sea").unwrap().materials[0]);
            }
            Err(e) => {
                panic!("Error reading brres file: {:#?}", e);
            }
        }
    }
}
