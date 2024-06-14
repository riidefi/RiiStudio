# brres
WIP Rust/C++ crate for reading BRRES files

BRRES (.brres) is Nintendo's first-party 3D model format for the Nintendo Wii. Used in games like *Mario Kart Wii*, *Twilight Princess* and *Super Smash Bros: Brawl*, .brres is a versatile and efficient file format.
Nearly all data is stored as raw GPU display lists that are directly read by the Wii's "flipper" GPU.

## File format: .brres

At the very root, ".brres" is an archive format for the following sub-files. All but SHP0 are supported.

Filetype   | Description                              | Rust structure
-----------|------------------------------------------|--------------------------
BRRES v0   | 3D Resource                              | [`Archive`](https://docs.rs/brres/latest/brres/struct.Archive.html)
MDL0 v11   | 3D Model                                 | [`Model`](https://docs.rs/brres/latest/brres/struct.Model.html)
TEX0 v1/v3 | Texture                                  | [`Texture`](https://docs.rs/brres/latest/brres/struct.Texture.html)
SRT0 v5    | Texture scale/rotate/translate animation | [`JSONSrtData`](https://docs.rs/brres/latest/brres/struct.JSONSrtData.html)
VIS0 v4    | Bone visibility animation                | [`JSONVisData`](https://docs.rs/brres/latest/brres/struct.JSONVisData.html)
CLR0 v4    | Shader uniform animation                 | [`JSONClrAnim`](https://docs.rs/brres/latest/brres/struct.JSONClrAnim.html), editing limitations
CHR0 v4    | Shader uniform animation                 | [`ChrData`](https://docs.rs/brres/latest/brres/struct.ChrData.html), editing limitations
PAT0 v4    | Texture image animation                  | [`JSONPatAnim`](https://docs.rs/brres/latest/brres/struct.JSONPatAnim.html), editing limitations
SHP0       | Bone/character animation                 | Unsupported

## File format: .mdl0

Filetype         | Description            | Rust structure
-----------------|------------------------|--------------
MDL0.ByteCode    | Draw calls + Skeleton  | Merged into [`JSONBoneData`](https://docs.rs/brres/latest/brres/json/struct.JSONBoneData.html), [`JSONDrawMatrix`](https://docs.rs/brres/latest/brres/json/struct.JSONDrawMatrix.html)
MDL0.Bone        | Bone                   | [`JSONBoneData`](https://docs.rs/brres/latest/brres/json/struct.JSONBoneData.html)
MDL0.PositionBuf | Holds vertex positions | [`VertexPositionBuffer`](https://docs.rs/brres/latest/brres/struct.VertexPositionBuffer.html)
MDL0.NormalBuf   | Holds vertex normals   | [`VertexNormalBuffer`](https://docs.rs/brres/latest/brres/struct.VertexNormalBuffer.html)
MDL0.ColorBuf    | Holds vertex colors    | [`VertexColorBuffer`](https://docs.rs/brres/latest/brres/struct.VertexColorBuffer.html)
MDL0.UVBuf       | Holds UV maps          | [`VertexTextureCoordinateBuffer`](https://docs.rs/brres/latest/brres/struct.VertexTextureCoordinateBuffer.html)
MDL0.FurVecBuf   | Fur related            | Not supported
MDL0.FurPosBuf   | Fur related            | Not supported
MDL0.Material    | Material data          | [`JSONMaterial`](https://docs.rs/brres/latest/brres/json/struct.JSONMaterial.html)
MDL0.TEV         | Shader data            | Merged into [`JSONMaterial`](https://docs.rs/brres/latest/brres/json/struct.JSONMaterial.html)
MDL0.Mesh        | 3D mesh data           | [`Mesh`](https://docs.rs/brres/latest/brres/struct.Mesh.html)
MDL0.TextureLink | Internal               | Recomputed
MDL0.PaletteLink | Internal               | Recomputed
MDL0.UserData    | Metadata               | Not supported

Thus, the Rust view of a MDL0 file looks like this:
```
use brres::*;
use brres::json::*;

struct Model {
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
```

## Reading a file
Read a .brres file from a path on the filesystem.

```
let archive = brres::Archive::from_path("kuribo.brres").unwrap();
assert!(archive.models[1].name == "kuribo");

println!("{:#?}", archive.get_model("kuribo").unwrap().meshes[0]);
```
Read a .brres file from a raw slice of bytes.

```
let raw_bytes = std::fs::read("kuribo.brres").expect("Expected kuribo :)");

let archive = brres::Archive::from_memory(&raw_bytes).unwrap();
assert!(archive.models[1].name == "kuribo");

println!("{:#?}", archive.get_model("kuribo").unwrap().meshes[0]);
```

## Writing a file
(to a file)
```
let kuribo = brres::Archive::from_path("kuribo.brres").unwrap();
let buf = kuribo.write_path("kuribo_modified.brres").unwrap();
```
(to memory)
```
let kuribo = brres::Archive::from_path("kuribo.brres").unwrap();
let buf = kuribo.write_memory().unwrap();
std::fs::write("kuribo_modified.brres", buf).unwrap();
```

## Examples
```rs
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
```

## Progress
Implements a Rust layer on top of `librii::g3d`'s JSON export-import layer. Importantly, large buffers like texture data and vertex data are not actually encoded in JSON but passed directly as a binary blob. This allows JSON files to stay light and results in minimal encoding latency (tests TBD).

| Format | Supported |
|--------|-----------|
| MDL0   | Yes       |
| TEX0   | Yes       |
| SRT0   | Yes       |
| PAT0   | Yes*      |
| CLR0   | Yes*      |
| CHR0   | Yes*      |
| VIS0   | Yes       |
| SHP0   | No        |

* Restrictions on track ordering when editing.

## Tests
Unit tests are being used to validate correctness. Run the suite with `cargo test`

## `brres-sys`
Low level documentation available [here](lib/brres-sys/README.md).
