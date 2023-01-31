# librii

## File Format Libraries

### librii::g3d
BRRES support.

#### 1. Testing
Every BRRES format is validated to produce 1:1 byte-identical output, even the Intermediate Representations (see below). Tests can be found in the `tests` folder.

#### 2. BRRES subfiles
Filetype   | Description                              | Source file       | Binary rep      | Intermediate rep
-----------|------------------------------------------|-------------------|-----------------|--------------
BRRES v0   | 3D Resource                              | ArchiveIO.cpp     | `BinaryArchive` | `Archive`
MDL0 v11   | 3D Model                                 | ModelIO.cpp       | `BinaryModel`   | `Model`
TEX0 v1/v3 | Texture                                  | TextureIO.cpp     | -               | `TextureData`
CLR0 v4    | Shader uniform animation                 | AnimClrIO.cpp     | `BinaryClr`     | -
SHP0       | Bone/character animation                 | -                 | -               | -
SRT0 v5    | Texture scale/rotate/translate animation | AnimIO.cpp        | `BinarySrt`     | `SrtAnim`\*
PAT0 v4    | Texture image animation                  | AnimTexPatIO.cpp  | `BinaryTexPat`  | -
VIS0 v4    | Bone visibility animation                | AnimVisIO.cpp     | `BinaryVis`     | -

\* `SrtAnim` is read-only for now; `BinaryArchive` contains read-write `BinarySrt` currently.

librii provides two structures for each filetype:
1. **Binary representation**: Designed to be a 1:1 mapping of the format. Editing this directly may require much book-keeping and be error prone. However, even corrupt models will usually be parseable here.
2. **Intermediate representation**: Simple enough to cover the entire set of officially-generated models but no simpler. Easy to edit; lots of validation. May fail on community models created with jank tooling.

Parsing a BRRES file into the **Binary representation** (`BinaryArchive`):
```cpp
std::expected<librii::g3d::BinaryArchive, std::string> parsed_brres = librii::g3d::BinaryArchive::read(reader, transaction);
if (!parsed_brres) {
    return std::unexpected(std::format("Failed to parse BRRES with error: {}", parsed_brres.error()));
}
```
Unpacking a BinaryArchive into the **Intermediate representation** (`Archive`):
```cpp
std::expected<librii::g3d::Archive, std::string> archive = librii::g3d::Archive::from(*parsed_brres);
if (!archive) {
    return std::unexpected(std::format("Failed to unpack BRRES file with error: {}", archive.error()));
}
librii::g3d::Archive data(std::move(*archive));
for (const auto &model : data.models) {
    printf("%s\n", std::format("Model: {} with {} materials", model.name, model.materials.size()).c_str());
}
```
#### 3. MDL0 subfiles
Information on MDL0 (`BinaryModel`, `Model`) specifically is provided:

Filetype         | Description            | Source file | Binary rep       | Intermediate rep
-----------------|------------------------|-------------|------------------|--------------
MDL0.ByteCode    | Draw calls + Skeleton  | ModelIO.cpp | `ByteCodeMethod` | Merged into `BoneData`, `DrawMatrix`
MDL0.Bone        | Bone                   | BoneIO.cpp  | `BinaryBoneData` | `BoneData`
MDL0.PositionBuf | Holds vertex positions | ModelIO.cpp | -                | `PositionBuffer`
MDL0.NormalBuf   | Holds vertex normals   | ModelIO.cpp | -                | `NormalBuffer`
MDL0.ColorBuf    | Holds vertex colors    | ModelIO.cpp | -                | `ColorBuffer`
MDL0.UVBuf       | Holds UV maps          | ModelIO.cpp | -                | `TextureCoordinateBuffer`
MDL0.FurVecBuf   | Fur related            | -           | -                | -
MDL0.FurPosBuf   | Fur related            | -           | -                | -
MDL0.Material    | Material data          | MatIO.cpp   | `BinaryMaterial` | `G3dMaterialData`
MDL0.TEV         | Shader data            | TevIO.cpp   | `BinaryTev` \*\* | Merged into `G3dMaterialData`
MDL0.Mesh        | 3D mesh data           | ModelIO.cpp | -                | `PolygonData`
MDL0.TextureLink | Internal               | ModelIO.cpp | -                | - \*\*\*
MDL0.PaletteLink | Internal               | -           | -                | - \*\*\*\*
MDL0.UserData    | Metadata               | -           | -                | -

\*\*\* `BinaryTev` is often replaced by a partial `gx::LowLevelGxMaterial` instance containing only the TEV fields `BinaryTev` sets.

\*\*\* Texture links are always recomputed. If a model had invalid texture links, simply passing the data through `BinaryModel` would correct it.

\*\*\*\* Palettes are not supported.

It's recommended you always use the intermediate representation when possible. `G3dMaterialData` directly satisfies `LowLevelGxMaterial` which makes it compatible with `librii::gl`'s shader compiler. Of course, a `BinaryMaterial` + `BinaryTev` (in the form of a partial `gx::LowLevelGxMaterial`) combo can be rendered via conversion with MatIO's function `fromBinMat(...) -> gx::LowLevelGxMaterial` with TEV merging enabled.

### librii::j3d
BMD/BDL support.

### librii::kcol
Support for KCollision v1 (.kcl) files across several platforms (DS, Wii, 3DS).

### librii::kmp
Support for Mario Kart Wii's .kmp files.

### librii::rhst
Interchange format designed specifically for GC/Wii. My blender exporter can produce these files.

### librii::assimp2rhst
Produces rhst files from an Assimp library mesh. Assimp supports reading dozens of formats including .dae, .fbx, etc.

### librii::szs
Support for compressing/decompressing SZS files.

## OpenGL Libraries

### librii::gl
OpenGL support.

### librii::glhelper
OpenGL helper classes. These can definitely use improvement.

### librii::gfx
C++ implementation of noclip's MegaState structure.

## Wii Graphics Libraries

### librii::gx
Data structures for the Wii graphics driver.

### librii::hx
Abstractions of librii::gx structures.

### librii::image
Supports encoding/decoding/resizing Wii images.

### librii::mtx
Texture matrix calculation.
NOTE: Lots of accuracy issues currently

## System/Hardware Libraries

### librii::nitro
Support for DS fixed-point types.

### librii::gpu
Allows direct interaction with the Wii GPU.

## Generic Libraries

### librii::math
3D math helpers: AABB, SRT3

