use serde::Deserialize;
use serde_json::Result;
use std::fs::File;
use std::io::BufReader;
use std::vec::Vec;

mod buffers;
mod ffi;

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
    materials: Vec<Option<serde_json::Value>>,
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
    format: i32,
    height: i32,
    maxLod: f32,
    minLod: f32,
    name: String,
    number_of_images: i32,
    sourcePath: String,
    width: i32,
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
    models: Vec<Model>,
    textures: Vec<Texture>,

    // Animation data
    chrs: Vec<serde_json::Value>,
    clrs: Vec<serde_json::Value>,
    pats: Vec<serde_json::Value>,
    srts: Vec<serde_json::Value>,
    viss: Vec<serde_json::Value>,
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
struct MatrixPrimitive {
    matrices: Vec<i32>,
    num_prims: i32,

    /*
    Stored as a giant buffer for convenience. To parse / edit it, the format is described below:

    Format:
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
        }

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
    vertex_data_buffer: Vec<u8>,
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
    name: String,

    visible: bool,

    // String (for now) references to vertex data arrays in the Model
    clr_buffer: Vec<String>,
    nrm_buffer: String,
    pos_buffer: String,
    uv_buffer: Vec<String>,

    // Bitfield of attributes (1 << n) for n in GXAttr enum
    vcd: i32,

    // For unrigged meshes which matrix (in the Model matrices list) are we referencing?
    current_matrix: i32,

    // Actual primitive data
    mprims: Vec<MatrixPrimitive>,
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
    name: String,

    bones: Vec<Option<serde_json::Value>>,
    materials: Vec<Option<serde_json::Value>>,
    meshes: Vec<Mesh>,

    // Vertex buffers
    positions: Vec<JsonBufferData>,
    normals: Vec<JsonBufferData>,
    texcoords: Vec<JsonBufferData>,
    colors: Vec<serde_json::Value>,

    // For skinning
    matrices: Vec<Option<serde_json::Value>>,
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
    name: String,

    // 0 to 1024
    width: i32,
    height: i32,

    // E.g. CMPR (14)
    format: i32,

    // 1 for no mipmaps.
    number_of_images: i32,

    // Raw GPU texture data. Use `gctex` or similar to decode
    data: Vec<u8>,

    // These are always recomputed on save (currently)
    max_lod: f32,
    min_lod: f32,
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
        let brres_data = fs::read("sea.brres").expect("Failed to read sea.brres file");

        // Call the read_raw_brres function
        match read_raw_brres(&brres_data) {
            Ok(archive) => {
                println!("{:#?}", archive.get_model("sea").unwrap().meshes[0]);
            }
            Err(e) => {
                panic!("Error reading brres file: {:#?}", e);
            }
        }
    }
}
