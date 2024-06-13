use std::fs::File;
use std::io::BufReader;

mod buffers;
mod enums;
mod json;

use brres_sys::ffi;
use enums::*;

use bytemuck;

use json::*;

/// Corresponds to a single `.brres` file
#[derive(Debug, Clone, PartialEq)]
pub struct Archive {
    /// 3D model data. Typically one per Archive, but multiple may appear here. For instance, a normal model, a LOD (low resolution) model and a shadow model.
    pub models: Vec<Model>,

    /// Textures shared by all models in the Archive.
    pub textures: Vec<Texture>,

    // Animation data
    /// Bone (character motion) animation
    pub chrs: Vec<ChrData>,

    /// Texture matrix ("SRT" a.k.a. "Scale, Rotate, Translate") animation
    pub srts: Vec<JSONSrtData>,

    /// Texture data (pattern) animation (e.g. like a GIF)
    pub pats: Vec<JSONPatAnim>,

    /// GPU uniform (color) animation
    pub clrs: Vec<serde_json::Value>,

    /// Bone visibility animation
    pub viss: Vec<serde_json::Value>,
}

/// Corresponds to a single MDL0 file
#[derive(Debug, Clone, PartialEq)]
pub struct Model {
    pub name: String,
    pub info: JSONModelInfo,

    pub bones: Vec<JSONBoneData>,
    pub materials: Vec<JSONMaterial>,
    pub meshes: Vec<Mesh>,

    // Vertex buffers
    pub positions: Vec<VertexPositionBuffer>,
    pub normals: Vec<VertexNormalBuffer>,
    pub texcoords: Vec<VertexTextureCoordinateBuffer>,
    pub colors: Vec<VertexColorBuffer>,

    /// For skinning
    pub matrices: Vec<JSONDrawMatrix>,
}

/// Holds triangle data
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

    /// Actual primitive data. In unrigged models, there will only be one MatrixPrimitive per Mesh.
    pub mprims: Vec<MatrixPrimitive>,
}

/// A group of triangles sharing the same skinning information. In unrigged models, there will only be one MatrixPrimitive per Mesh.
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

/// Holds positions of vertices in 3D space.
#[derive(Debug, Clone, PartialEq)]
pub struct VertexPositionBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[f32; 3]>,
    /// Only here to make unit tests simple (1:1 matching)
    cached_minmax: Option<([f32; 3], [f32; 3])>,
}

/// Holds normal vectors of vertices in 3D space.
#[derive(Debug, Clone, PartialEq)]
pub struct VertexNormalBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[f32; 3]>,
    /// Only here to make unit tests simple (1:1 matching)
    cached_minmax: Option<([f32; 3], [f32; 3])>,
}

/// Holds vertex colors.
#[derive(Debug, Clone, PartialEq)]
pub struct VertexColorBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[u8; 4]>,
    /// Only here to make unit tests simple (1:1 matching)
    cached_minmax: Option<([u8; 4], [u8; 4])>,
}

/// Holds vertex UV coordinates.
#[derive(Debug, Clone, PartialEq)]
pub struct VertexTextureCoordinateBuffer {
    id: i32,
    name: String,
    q_comp: i32,
    q_divisor: i32,
    q_stride: i32,
    q_type: i32,
    data: Vec<[f32; 2]>,
    /// Only here to make unit tests simple (1:1 matching)
    cached_minmax: Option<([f32; 2], [f32; 2])>,
}

/// Holds a hardware-encoded image. Potentially may store multiple mip levels in a hierarchy. See https://en.wikipedia.org/wiki/Mipmap#Mechanism
#[derive(Debug, Clone, PartialEq)]
pub struct Texture {
    /// Name of this resource.
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

/// Bone (character motion) animation
#[derive(Debug, Clone, PartialEq)]
pub struct ChrData {
    /// Name of this resource.
    name: String,

    /// Bones/Joints/Nodes that are being animated. Specifies which attributes of a Bone are animated.
    nodes: Vec<ChrNode>,

    /// Actual animation curve data.
    tracks: Vec<ChrTrack>,

    source_path: String,
    frame_duration: u16,
    wrap_mode: u32,
    scale_rule: u32,
}

/// Describes the animations being applied to a single Bone.
#[derive(Debug, Clone, PartialEq)]
pub struct ChrNode {
    /// Name of the Bone to link to.
    name: String,

    /// TODO: Should not be exposed to the user.
    flags: u32,

    /// You'll need to interpret `flags` to determine which track indices correspond to which attributes, for now.
    tracks: Vec<u32>,
}

/// Animation curve data.
#[derive(Debug, Clone, PartialEq)]
pub struct ChrTrack {
    /// How is this curve being stored? Different formats offer different (lossy) compression.
    /// ChrQuantization::Constant is special--when specified only a single keyframe is permitted.
    quant: ChrQuantization,
    /// For some animation formats, you will need to compute an optimal scale/offset affine transform that reduces the signed area of the curve.
    scale: f32,
    /// For some animation formats, you will need to compute an optimal scale/offset affine transform that reduces the signed area of the curve.
    offset: f32,
    /// This *should* be recomputed--we have this here for unit tests with BrawlBox. Should be made Optional.
    step: f32,
    /// Keyframes for the Curve data.
    frames_data: Vec<ChrFrame>,
}

/// Hermine spline point in an animation curve. We use f64 because the quantized fixed-point numbers have far more precision than f32 allows (and without this, we wouldn't currently support 1:1 matching on encode-decode.)
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ChrFrame {
    /// x: Position of the current keyframe (in time)
    frame: f64,
    /// f(x): Curve evaluated at the frame
    value: f64,
    /// f'(x): Slope of curve at the frame
    slope: f64,
}

// Useful structure for dumping blobs and keeping track of indices.
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

impl Archive {
    fn from_json(archive: JsonArchive, buffers: Vec<Vec<u8>>) -> Self {
        Self {
            chrs: archive
                .chrs
                .into_iter()
                .map(|m| ChrData::from_json(m, &buffers))
                .collect(),
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

    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonArchive {
        JsonArchive {
            chrs: self.chrs.iter().map(|c| c.to_json(ctx)).collect(),
            clrs: self.clrs.clone(),
            models: self.models.iter().map(|m| m.to_json(ctx)).collect(),
            pats: self.pats.clone(),
            srts: self.srts.clone(),
            textures: self.textures.iter().map(|t| t.to_json(ctx)).collect(),
            viss: self.viss.clone(),
        }
    }
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
                .map(|m| VertexPositionBuffer::from_json(m, buffers))
                .collect(),
            normals: model
                .normals
                .into_iter()
                .map(|m| VertexNormalBuffer::from_json(m, buffers))
                .collect(),
            colors: model
                .colors
                .into_iter()
                .map(|m| VertexColorBuffer::from_json(m, buffers))
                .collect(),
            texcoords: model
                .texcoords
                .into_iter()
                .map(|m| VertexTextureCoordinateBuffer::from_json(m, buffers))
                .collect(),
        }
    }

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

    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JsonPrimitive {
        let buffer_id = ctx.save_buffer_with_copy(&self.vertex_data_buffer);

        JsonPrimitive {
            matrices: self.matrices.clone(),
            num_prims: self.num_prims,
            vertexDataBufferId: buffer_id as i32,
        }
    }
}

impl VertexPositionBuffer {
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

impl VertexNormalBuffer {
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

impl VertexColorBuffer {
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

impl VertexTextureCoordinateBuffer {
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

impl ChrData {
    fn from_json(json_data: JSONChrData, buffers: &[Vec<u8>]) -> Self {
        let nodes = json_data
            .nodes
            .into_iter()
            .map(ChrNode::from_json)
            .collect();
        let tracks = json_data
            .tracks
            .into_iter()
            .map(|json_track| ChrTrack::from_json(json_track, buffers))
            .collect();

        Self {
            nodes,
            tracks,
            name: json_data.name,
            source_path: json_data.sourcePath,
            frame_duration: json_data.frameDuration,
            wrap_mode: json_data.wrapMode,
            scale_rule: json_data.scaleRule,
        }
    }

    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JSONChrData {
        let nodes = self.nodes.iter().map(ChrNode::to_json).collect();
        let tracks = self.tracks.iter().map(|track| track.to_json(ctx)).collect();

        JSONChrData {
            nodes,
            tracks,
            name: self.name.clone(),
            sourcePath: self.source_path.clone(),
            frameDuration: self.frame_duration,
            wrapMode: self.wrap_mode,
            scaleRule: self.scale_rule,
        }
    }
}

impl ChrNode {
    fn from_json(json_node: JSONChrNode) -> Self {
        Self {
            name: json_node.name,
            flags: json_node.flags,
            tracks: json_node.tracks,
        }
    }

    fn to_json(&self) -> JSONChrNode {
        JSONChrNode {
            name: self.name.clone(),
            flags: self.flags,
            tracks: self.tracks.clone(),
        }
    }
}

impl ChrTrack {
    fn from_json(json_track: JSONChrTrack, buffers: &[Vec<u8>]) -> Self {
        let data = buffers
            .get(json_track.framesDataBufferId as usize)
            .expect("Invalid buffer ID");
        let frames_data = ChrFramePacker::unpack(data, json_track.numKeyFrames as usize);

        Self {
            quant: json_track.quant,
            scale: json_track.scale,
            offset: json_track.offset,
            step: json_track.step,
            frames_data,
        }
    }

    fn to_json(&self, ctx: &mut JsonWriteCtx) -> JSONChrTrack {
        let buffer_id = ctx.save_buffer_with_move(ChrFramePacker::pack(&self.frames_data));

        JSONChrTrack {
            quant: self.quant,
            scale: self.scale,
            offset: self.offset,
            step: self.step,
            framesDataBufferId: buffer_id as u32,
            numKeyFrames: self.frames_data.len() as u32,
        }
    }
}

struct ChrFramePacker;

impl ChrFramePacker {
    const CHR_FRAME_SIZE: usize = 8 * 3;

    pub fn pack(frames: &[ChrFrame]) -> Vec<u8> {
        let mut buffer = Vec::with_capacity(frames.len() * Self::CHR_FRAME_SIZE);
        for frame in frames {
            buffer.extend_from_slice(&frame.frame.to_le_bytes());
            buffer.extend_from_slice(&frame.value.to_le_bytes());
            buffer.extend_from_slice(&frame.slope.to_le_bytes());
        }
        buffer
    }

    pub fn unpack(bytes: &[u8], num_keyframes: usize) -> Vec<ChrFrame> {
        let mut frames = Vec::with_capacity(num_keyframes);
        for i in 0..num_keyframes {
            let offset = i * Self::CHR_FRAME_SIZE;
            let frame = f64::from_le_bytes(bytes[offset..offset + 8].try_into().unwrap());
            let value = f64::from_le_bytes(bytes[offset + 8..offset + 16].try_into().unwrap());
            let slope = f64::from_le_bytes(bytes[offset + 16..offset + 24].try_into().unwrap());
            frames.push(ChrFrame {
                frame,
                value,
                slope,
            });
        }
        frames
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
        let name = std::path::Path::new(path)
            .file_name()
            .unwrap()
            .to_str()
            .unwrap();
        let brres_data = fs::read(path).expect("Failed to read brres file");
        let archive = read_raw_brres(&brres_data).expect("Error reading brres file");
        let written_data = write_raw_brres(&archive).expect("Error writing brres file");

        if passes {
            if &brres_data != &written_data {
                fs::write(format!("{name}_good.brres"), &brres_data);
                fs::write(format!("{name}_bad.brres"), &written_data);
            }
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
        test_validate_binary_is_lossless("../../tests/samples/sea.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_srt() {
        test_validate_binary_is_lossless("../../tests/samples/srt.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_rPB() {
        test_validate_binary_is_lossless("../../tests/samples/rPB.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_MashBalloonGC() {
        test_validate_binary_is_lossless("../../tests/samples/MashBalloonGC.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_human_walk_env() {
        test_validate_binary_is_lossless("../../tests/samples/human_walk_env.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_map_model() {
        test_validate_binary_is_lossless("../../tests/samples/map_model.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_kuribo() {
        // CHR0
        test_validate_binary_is_lossless("../../tests/samples/kuribo.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_human_walk_chr0() {
        test_validate_binary_is_lossless("../../tests/samples/human_walk_chr0.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_driver_model() {
        test_validate_binary_is_lossless("../../tests/samples/driver_model.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_mariotreeGC() {
        test_validate_binary_is_lossless("../../tests/samples/mariotreeGC.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_fur_rabbits_chr0() {
        test_validate_binary_is_lossless("../../tests/samples/fur_rabbits-chr0.brres", true)
    }
    #[test]
    fn test_validate_binary_is_lossless_luigi_circuit() {
        // PAT0
        test_validate_binary_is_lossless("../../tests/samples/luigi_circuit.brres", true)
    }
}
