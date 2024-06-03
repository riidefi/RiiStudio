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
struct Archive {
    chrs: Vec<serde_json::Value>,
    clrs: Vec<serde_json::Value>,
    models: Vec<Model>,
    pats: Vec<serde_json::Value>,
    srts: Vec<serde_json::Value>,
    textures: Vec<Texture>,
    viss: Vec<serde_json::Value>,
    buffers: Vec<Vec<u8>>,
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
            buffers,
        }
    }

    fn get_model(&self, name: &str) -> Option<&Model> {
        self.models.iter().find(|model| model.name == name)
    }

    fn get_texture(&self, name: &str) -> Option<&Texture> {
        self.textures.iter().find(|texture| texture.name == name)
    }

    fn get_buffer(&self, id: usize) -> Option<&[u8]> {
        self.buffers.get(id).map(|buffer| buffer.as_slice())
    }
}

#[derive(Debug)]
struct Primitive {
    matrices: Vec<i32>,
    num_prims: i32,
    vertex_data_buffer: Vec<u8>,
}

impl Primitive {
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
struct Mesh {
    clr_buffer: Vec<String>,
    current_matrix: i32,
    mprims: Vec<Primitive>,
    name: String,
    nrm_buffer: String,
    pos_buffer: String,
    uv_buffer: Vec<String>,
    vcd: i32,
    visible: bool,
}

impl Mesh {
    fn from_json(json_mesh: JsonMesh, buffers: &[Vec<u8>]) -> Self {
        Self {
            clr_buffer: json_mesh.clr_buffer,
            current_matrix: json_mesh.current_matrix,
            mprims: json_mesh
                .mprims
                .into_iter()
                .map(|p| Primitive::from_json(p, buffers))
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
struct Model {
    bones: Vec<Option<serde_json::Value>>,
    colors: Vec<serde_json::Value>,
    materials: Vec<Option<serde_json::Value>>,
    matrices: Vec<Option<serde_json::Value>>,
    meshes: Vec<Mesh>,
    name: String,
    normals: Vec<JsonBufferData>,
    positions: Vec<JsonBufferData>,
    texcoords: Vec<JsonBufferData>,
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
struct Texture {
    data: Vec<u8>,
    format: i32,
    height: i32,
    max_lod: f32,
    min_lod: f32,
    name: String,
    number_of_images: i32,
    width: i32,
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
                assert!(archive.get_buffer(0).is_some());
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
    let tmp = ffi::CBrresWrapper::from_bytes(brres);

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
                println!("{:?}", archive);
            }
            Err(e) => {
                panic!("Error reading brres file: {:?}", e);
            }
        }
    }
}
