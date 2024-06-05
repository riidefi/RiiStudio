use serde::Deserialize;
use serde::Serialize;
use std::fs::File;
use std::io::BufReader;

mod buffers;
mod enums;

use brres_sys::ffi;
use enums::*;

use bytemuck;

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONMatrixWeight {
    bone_id: u32,
    weight: f32,
}

impl Default for JSONMatrixWeight {
    fn default() -> Self {
        JSONMatrixWeight {
            bone_id: 0,
            weight: 1.0,
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONDrawMatrix {
    weights: Vec<JSONMatrixWeight>,
}

impl Default for JSONDrawMatrix {
    fn default() -> Self {
        JSONDrawMatrix {
            weights: Vec::new(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONModelInfo {
    scaling_rule: String,
    texmtx_mode: String,
    source_location: String,
    evpmtx_mode: String,
}

impl Default for JSONModelInfo {
    fn default() -> Self {
        JSONModelInfo {
            scaling_rule: "Maya".to_string(),
            texmtx_mode: "Maya".to_string(),
            source_location: "".to_string(),
            evpmtx_mode: "Normal".to_string(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
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

#[derive(Serialize, Deserialize, Debug, Clone)]
struct JsonPrimitive {
    matrices: Vec<i32>,
    num_prims: i32,
    vertexDataBufferId: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct JsonModel {
    name: String,
    info: JSONModelInfo,
    bones: Vec<JSONBoneData>,
    materials: Vec<JSONMaterial>,
    matrices: Vec<JSONDrawMatrix>,
    meshes: Vec<JsonMesh>,
    positions: Vec<JsonBufferData<[f32; 3]>>,
    normals: Vec<JsonBufferData<[f32; 3]>>,
    colors: Vec<JsonBufferData<[u8; 4]>>,
    texcoords: Vec<JsonBufferData<[f32; 2]>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct JsonBufferData<T> {
    dataBufferId: i32,
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    #[serde(skip_serializing_if = "Option::is_none")]
    cached_min: Option<T>,
    #[serde(skip_serializing_if = "Option::is_none")]
    cached_max: Option<T>,
}

#[derive(Debug, Clone, PartialEq)]
struct PositionBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[f32; 3]>,
    cached_minmax: Option<([f32; 3], [f32; 3])>,
}

#[derive(Debug, Clone, PartialEq)]
struct NormalBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[f32; 3]>,
    cached_minmax: Option<([f32; 3], [f32; 3])>,
}

#[derive(Debug, Clone, PartialEq)]
struct ColorBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[u8; 4]>,
    cached_minmax: Option<([u8; 4], [u8; 4])>,
}

#[derive(Debug, Clone, PartialEq)]
struct TextureCoordinateBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[f32; 2]>,
    cached_minmax: Option<([f32; 2], [f32; 2])>,
}

impl PositionBuffer {
    fn from_json(buffer_data: JsonBufferData<[f32; 3]>, buffers: &[Vec<u8>]) -> Self {
        let data = buffers
            .get(buffer_data.dataBufferId as usize)
            .expect("Invalid buffer ID");
        let elem_count = data.len() / std::mem::size_of::<[f32; 3]>();
        let data: Vec<[f32; 3]> = unsafe {
            std::slice::from_raw_parts(data.as_ptr() as *const [f32; 3], elem_count).to_vec()
        };

        let cached_minmax = if let (Some(cached_min), Some(cached_max)) =
            (buffer_data.cached_min, buffer_data.cached_max)
        {
            Some((cached_min, cached_max))
        } else {
            None
        };

        Self {
            id: buffer_data.id,
            name: buffer_data.name,
            q_comp: buffer_data.q_comp,
            q_divisor: buffer_data.q_divisor,
            q_stride: buffer_data.q_stride,
            q_type: buffer_data.q_type,
            data,
            cached_minmax,
        }
    }
}

impl NormalBuffer {
    fn from_json(buffer_data: JsonBufferData<[f32; 3]>, buffers: &[Vec<u8>]) -> Self {
        let data = buffers
            .get(buffer_data.dataBufferId as usize)
            .expect("Invalid buffer ID");
        let elem_count = data.len() / std::mem::size_of::<[f32; 3]>();
        let data: Vec<[f32; 3]> = unsafe {
            std::slice::from_raw_parts(data.as_ptr() as *const [f32; 3], elem_count).to_vec()
        };

        let cached_minmax = if let (Some(cached_min), Some(cached_max)) =
            (buffer_data.cached_min, buffer_data.cached_max)
        {
            Some((cached_min, cached_max))
        } else {
            None
        };

        Self {
            id: buffer_data.id,
            name: buffer_data.name,
            q_comp: buffer_data.q_comp,
            q_divisor: buffer_data.q_divisor,
            q_stride: buffer_data.q_stride,
            q_type: buffer_data.q_type,
            data,
            cached_minmax,
        }
    }
}

impl ColorBuffer {
    fn from_json(buffer_data: JsonBufferData<[u8; 4]>, buffers: &[Vec<u8>]) -> Self {
        let data = buffers
            .get(buffer_data.dataBufferId as usize)
            .expect("Invalid buffer ID");
        let elem_count = data.len() / std::mem::size_of::<[u8; 4]>();
        let data: Vec<[u8; 4]> = unsafe {
            std::slice::from_raw_parts(data.as_ptr() as *const [u8; 4], elem_count).to_vec()
        };

        let cached_minmax = if let (Some(cached_min), Some(cached_max)) =
            (buffer_data.cached_min, buffer_data.cached_max)
        {
            Some((cached_min, cached_max))
        } else {
            None
        };

        Self {
            id: buffer_data.id,
            name: buffer_data.name,
            q_comp: buffer_data.q_comp,
            q_divisor: buffer_data.q_divisor,
            q_stride: buffer_data.q_stride,
            q_type: buffer_data.q_type,
            data,
            cached_minmax,
        }
    }
}

impl TextureCoordinateBuffer {
    fn from_json(buffer_data: JsonBufferData<[f32; 2]>, buffers: &[Vec<u8>]) -> Self {
        let data = buffers
            .get(buffer_data.dataBufferId as usize)
            .expect("Invalid buffer ID");
        let elem_count = data.len() / std::mem::size_of::<[f32; 2]>();
        let data: Vec<[f32; 2]> = unsafe {
            std::slice::from_raw_parts(data.as_ptr() as *const [f32; 2], elem_count).to_vec()
        };

        let cached_minmax = if let (Some(cached_min), Some(cached_max)) =
            (buffer_data.cached_min, buffer_data.cached_max)
        {
            Some((cached_min, cached_max))
        } else {
            None
        };

        Self {
            id: buffer_data.id,
            name: buffer_data.name,
            q_comp: buffer_data.q_comp,
            q_divisor: buffer_data.q_divisor,
            q_stride: buffer_data.q_stride,
            q_type: buffer_data.q_type,
            data,
            cached_minmax,
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
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

#[derive(Serialize, Deserialize, Debug, Clone)]
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

#[derive(Debug, Clone, PartialEq)]
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

#[derive(Debug, Clone, PartialEq)]
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

#[derive(Debug, Clone, PartialEq)]
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

#[derive(Debug, Clone, PartialEq)]
pub struct Model {
    pub name: String,
    pub info: JSONModelInfo,

    pub bones: Vec<JSONBoneData>,
    pub materials: Vec<JSONMaterial>,
    pub meshes: Vec<Mesh>,

    // Vertex buffers
    pub positions: Vec<PositionBuffer>,
    pub normals: Vec<NormalBuffer>,
    pub texcoords: Vec<TextureCoordinateBuffer>,
    pub colors: Vec<ColorBuffer>,

    /// For skinning
    pub matrices: Vec<JSONDrawMatrix>,
}

impl Model {
    fn from_json(model: JsonModel, buffers: &[Vec<u8>]) -> Self {
        Self {
            bones: model.bones,
            materials: model.materials,
            matrices: model.matrices,
            meshes: model
                .meshes
                .into_iter()
                .map(|m| Mesh::from_json(m, buffers))
                .collect(),
            name: model.name,
            info: model.info,
            positions: model
                .positions
                .into_iter()
                .map(|m| PositionBuffer::from_json(m, buffers))
                .collect(),
            normals: model
                .normals
                .into_iter()
                .map(|m| NormalBuffer::from_json(m, buffers))
                .collect(),
            colors: model
                .colors
                .into_iter()
                .map(|m| ColorBuffer::from_json(m, buffers))
                .collect(),
            texcoords: model
                .texcoords
                .into_iter()
                .map(|m| TextureCoordinateBuffer::from_json(m, buffers))
                .collect(),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONDrawCall {
    material: u32,
    poly: u32,
    prio: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONBoneData {
    name: String,
    ssc: bool,
    visible: bool,
    billboard_type: u32,
    scale: [f32; 3],
    rotate: [f32; 3],
    translate: [f32; 3],
    volume_min: [f32; 3],
    volume_max: [f32; 3],
    parent: i32,
    children: Vec<i32>,
    display_matrix: bool,
    draw_calls: Vec<JSONDrawCall>,
    force_display_matrix: bool,
    omit_from_node_mix: bool,
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct Color {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct ColorS10 {
    r: i16,
    g: i16,
    b: i16,
    a: i16,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
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

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONChannelControl {
    enabled: bool,
    Ambient: ColorSource,
    Material: ColorSource,
    lightMask: LightID,
    diffuseFn: DiffuseFunction,
    attenuationFn: AttenuationFunction,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONChannelData {
    matColor: Color,
    ambColor: Color,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
struct JSONTexCoordGen {
    func: TexGenType,
    sourceParam: TexGenSrc,
    matrix: TexMatrix,
    normalize: bool,
    postMatrix: PostTexMatrix,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
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

struct JsonWriteCtx {
    buffers: Vec<Vec<u8>>,
}

impl JsonWriteCtx {
    fn new() -> Self {
        JsonWriteCtx {
            buffers: Vec::new(),
        }
    }

    fn save_buffer_with_move(&mut self, buf: Vec<u8>) -> usize {
        self.buffers.push(buf);
        self.buffers.len() - 1
    }

    fn save_buffer_with_copy<T: AsRef<[u8]>>(&mut self, buf: T) -> usize {
        let tmp: Vec<u8> = buf.as_ref().to_vec();
        self.save_buffer_with_move(tmp)
    }
}
impl PositionBuffer {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonBufferData<[f32; 3]> {
        let buffer_id = ctx.save_buffer_with_copy(bytemuck::cast_slice(&self.data));

        JsonBufferData {
            dataBufferId: buffer_id as i32,
            id: self.id,
            name: self.name.clone(),
            q_comp: self.q_comp,
            q_divisor: self.q_divisor,
            q_stride: self.q_stride,
            q_type: self.q_type,
            cached_min: self.cached_minmax.map(|(min, _)| min),
            cached_max: self.cached_minmax.map(|(_, max)| max),
        }
    }
}

impl NormalBuffer {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonBufferData<[f32; 3]> {
        let buffer_id = ctx.save_buffer_with_copy(bytemuck::cast_slice(&self.data));

        JsonBufferData {
            dataBufferId: buffer_id as i32,
            id: self.id,
            name: self.name.clone(),
            q_comp: self.q_comp,
            q_divisor: self.q_divisor,
            q_stride: self.q_stride,
            q_type: self.q_type,
            cached_min: self.cached_minmax.map(|(min, _)| min),
            cached_max: self.cached_minmax.map(|(_, max)| max),
        }
    }
}

impl ColorBuffer {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonBufferData<[u8; 4]> {
        let buffer_id = ctx.save_buffer_with_copy(bytemuck::cast_slice(&self.data));

        JsonBufferData {
            dataBufferId: buffer_id as i32,
            id: self.id,
            name: self.name.clone(),
            q_comp: self.q_comp,
            q_divisor: self.q_divisor,
            q_stride: self.q_stride,
            q_type: self.q_type,
            cached_min: self.cached_minmax.map(|(min, _)| min),
            cached_max: self.cached_minmax.map(|(_, max)| max),
        }
    }
}

impl TextureCoordinateBuffer {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonBufferData<[f32; 2]> {
        let buffer_id = ctx.save_buffer_with_copy(bytemuck::cast_slice(&self.data));

        JsonBufferData {
            dataBufferId: buffer_id as i32,
            id: self.id,
            name: self.name.clone(),
            q_comp: self.q_comp,
            q_divisor: self.q_divisor,
            q_stride: self.q_stride,
            q_type: self.q_type,
            cached_min: self.cached_minmax.map(|(min, _)| min),
            cached_max: self.cached_minmax.map(|(_, max)| max),
        }
    }
}

impl MatrixPrimitive {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonPrimitive {
        let buffer_id = ctx.save_buffer_with_copy(&self.vertex_data_buffer);

        JsonPrimitive {
            matrices: self.matrices.clone(),
            num_prims: self.num_prims,
            vertexDataBufferId: buffer_id as i32,
        }
    }
}

impl Mesh {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonMesh {
        JsonMesh {
            clr_buffer: self.clr_buffer.clone(),
            current_matrix: self.current_matrix,
            mprims: self.mprims.iter().map(|p| p.to_json(ctx)).collect(),
            name: self.name.clone(),
            nrm_buffer: self.nrm_buffer.clone(),
            pos_buffer: self.pos_buffer.clone(),
            uv_buffer: self.uv_buffer.clone(),
            vcd: self.vcd,
            visible: self.visible,
        }
    }
}

impl Model {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonModel {
        JsonModel {
            name: self.name.clone(),
            info: self.info.clone(),
            bones: self.bones.clone(),
            materials: self.materials.clone(),
            matrices: self.matrices.clone(),
            meshes: self.meshes.iter().map(|m| m.to_json(ctx)).collect(),
            positions: self.positions.iter().map(|p| p.to_json(ctx)).collect(),
            normals: self.normals.iter().map(|n| n.to_json(ctx)).collect(),
            colors: self.colors.iter().map(|c| c.to_json(ctx)).collect(),
            texcoords: self.texcoords.iter().map(|t| t.to_json(ctx)).collect(),
        }
    }
}

impl Texture {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonTexture {
        let buffer_id = ctx.save_buffer_with_copy(&self.data);

        JsonTexture {
            dataBufferId: buffer_id as i32,
            format: self.format,
            height: self.height,
            maxLod: self.max_lod,
            minLod: self.min_lod,
            name: self.name.clone(),
            number_of_images: self.number_of_images,
            width: self.width,
            sourcePath: "".to_string(),
        }
    }
}

impl Archive {
    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonArchive {
        JsonArchive {
            chrs: self.chrs.clone(),
            clrs: self.clrs.clone(),
            models: self.models.iter().map(|m| m.to_json(ctx)).collect(),
            pats: self.pats.clone(),
            srts: self.srts.clone(),
            textures: self.textures.iter().map(|t| t.to_json(ctx)).collect(),
            viss: self.viss.clone(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    #[test]
    fn test_archive() {
        let file_path = "dummy.json";
        match read_json(file_path) {
            Ok(json_archive) => {
                // Assuming buffers are loaded here
                let buffers = buffers::read_buffer(&fs::read("dummy.bin").unwrap());
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

pub fn create_json(archive: &Archive) -> anyhow::Result<(String, Vec<u8>)> {
    let mut ctx = JsonWriteCtx::new();
    let json_archive = archive.to_json(&mut ctx);
    let json_str = serde_json::to_string_pretty(&json_archive)?;
    let blob = buffers::collate_buffers(&ctx.buffers);

    Ok((json_str, blob))
}

pub fn read_raw_brres(brres: &[u8]) -> anyhow::Result<Archive> {
    let tmp = ffi::CBrresWrapper::from_bytes(brres)?;

    create_archive(&tmp.json_metadata, &tmp.buffer_data)
}

pub fn write_raw_brres(archive: &Archive) -> anyhow::Result<Vec<u8>> {
    let (json, blob) = create_json(&archive)?;

    let ffi_obj = ffi::CBrresWrapper::write_bytes(&json, &blob)?;

    // This is a copy we could avoid by returning the CBrresWrapper iteslf--but this
    // avoids exposing a dependency and keeps the API simple.
    Ok(ffi_obj.buffer_data.to_vec())
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

    #[test]
    fn test_validate_json_is_lossless_sea() {
        // Read the sea.brres file into a byte array
        let brres_data =
            fs::read("../../tests/samples/sea.brres").expect("Failed to read sea.brres file");

        // Call the read_raw_brres function to create the initial Archive
        let initial_archive = read_raw_brres(&brres_data).expect("Failed to read brres file");

        // Convert the initial Archive to a JSON string
        let (json_str, blob) =
            create_json(&initial_archive).expect("Failed to create JSON from Archive");

        // Create a new Archive from the JSON string
        let new_archive =
            create_archive(&json_str, &blob).expect("Failed to create Archive from JSON");

        // new_archive.models[0].name = "lol".to_string();

        // Compare the initial and new Archives (you might need to implement PartialEq for Archive)
        assert_eq!(
            initial_archive, new_archive,
            "Archives do not match after JSON roundtrip"
        );

        // Print some debug information if needed
        println!("{:#?}", initial_archive.get_model("sea").unwrap().meshes[0]);
        println!(
            "{:#?}",
            initial_archive.get_model("sea").unwrap().materials[0]
        );
    }
    fn assert_buffers_equal(encoded_image: &[u8], cached_image: &[u8], format: &str) {
        if encoded_image != cached_image {
            let diff_count = encoded_image
                .iter()
                .zip(cached_image.iter())
                .filter(|(a, b)| a != b)
                .count();
            println!("Mismatch for {:?} - {} bytes differ", format, diff_count);

            // Print some example differences (up to 10)
            let mut diff_examples = vec![];
            for (i, (a, b)) in encoded_image.iter().zip(cached_image.iter()).enumerate() {
                if a != b {
                    diff_examples.push((i, *a, *b));
                    if diff_examples.len() >= 10 {
                        break;
                    }
                }
            }

            println!("Example differences (up to 10):");
            for (i, a, b) in diff_examples {
                println!("Byte {}: encoded = {}, cached = {}", i, a, b);
            }

            println!(
                "Size of left: {}, size of right: {}",
                encoded_image.len(),
                cached_image.len()
            );
            panic!("Buffers are not equal");
        }
    }
    fn test_validate_binary_is_lossless(path: &str, passes: bool) {
        let brres_data = fs::read(path).expect("Failed to read brres file");
        let archive = read_raw_brres(&brres_data).expect("Error reading brres file");
        let written_data = write_raw_brres(&archive).expect("Error writing brres file");

        if passes {
            assert_buffers_equal(&brres_data, &written_data, path);
        } else {
            assert!(&brres_data != &written_data)
        }
    }
    #[test]
    fn test_validate_binary_is_lossless_walk() {
        let brres_data = fs::read("../../tests/samples/human_walk.brres")
            .expect("Failed to read human_walk.brres file");
        let archive = read_raw_brres(&brres_data).expect("Error reading brres file");
        let written_data = write_raw_brres(&archive).expect("Error writing brres file");

        fs::write("Invalid_human_walk.brres", &written_data).unwrap();

        // panic!("{:#?}", archive.get_model("human_walk").unwrap().materials[0]);

        assert_buffers_equal(&brres_data, &written_data, "human_walk.brres");
    }
    #[test]
    fn test_validate_binary_is_lossless_sea() {
        // FAILS: SRT0
        test_validate_binary_is_lossless("../../tests/samples/sea.brres", false)
    }
    #[test]
    fn test_validate_binary_is_lossless_map_model() {
        // FAILS: ???
        test_validate_binary_is_lossless("../../tests/samples/map_model.brres", false)
    }
}
