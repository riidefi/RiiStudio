[![crates.io](https://img.shields.io/crates/v/brres-sys.svg)](https://crates.io/crates/brres-sys)
[![docs.rs](https://docs.rs/brres-sys/badge.svg)](https://docs.rs/brres-sys/)

# `brres-sys`

Implements a Rust layer on top of `librii::g3d`'s JSON export-import layer. Importantly, large buffers like texture data and vertex data are not actually encoded in JSON but passed directly as a binary blob. This allows JSON files to stay light.

Exposes the following Rust interface
```rs
pub struct CBrresWrapper<'a> {
    pub json_metadata: &'a str,
    pub buffer_data: &'a [u8],
    // ...
}

impl<'a> CBrresWrapper<'a> {
	// .bin -> .json
    pub fn from_bytes(buf: &[u8]) -> anyhow::Result<Self>;
    // .json -> .bin
    pub fn write_bytes(json: &str, buffer: &[u8]) -> anyhow::Result<Self>;
}
```

This wraps the following C interface, which can still be used (say for other language bindings).
```c
// include/brres_sys.h
struct CResult {
	const char* json_metadata;
	uint32_t len_json_metadata;
	const void* buffer_data;
	uint32_t len_buffer_data;
	void (*freeResult)(struct CResult* self);
	void* opaque;
};
uint32_t brres_read_from_bytes(CResult* result, const void* buf,
                               uint32_t len);
uint32_t brres_write_bytes(CResult* result, const char* json,
                          uint32_t json_len, const void* buffer,
                          uint32_t buffer_len);
void brres_free(CResult* result);
```

## Internal API docs

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

## Example usage (low level API)

e.g. the following idiomatic C code:
```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "include/brres_sys.h"

int main() {
	uint8_t my_brres_file[] = { /* ... */ }; // Initialize with actual .brres file data

	CResult result = {};
	uint32_t ok = brres_read_from_bytes(&result, my_brres_file, sizeof(my_brres_file));

	if (!ok) {
		printf("Failed to read .brres: %*.s\n", (size_t)result.len_json_metadata, result.json_metadata);
		brres_free(&result); // Free the error message
		return 1;
	}

	// Write JSON metadata to a file
	FILE *json_file = fopen("output.json", "wb");
	if (json_file == NULL) {
		perror("Failed to open output.json for writing");
		brres_free(&result);
		return 1;
	}

	size_t written = fwrite(result.json_metadata, 1, result.len_json_metadata, json_file);
	if (written != result.len_json_metadata) {
		perror("Failed to write all JSON metadata to output.json");
		fclose(json_file);
		brres_free(&result);
		return 1;
	}

	fclose(json_file);

	// Write binary buffer data to a file
	FILE *bin_file = fopen("output.bin", "wb");
	if (bin_file == NULL) {
		perror("Failed to open output.bin for writing");
		brres_free(&result);
		return 1;
	}

	written = fwrite(result.buffer_data, 1, result.len_buffer_data, bin_file);
	if (written != result.len_buffer_data) {
		perror("Failed to write all buffer data to output.bin");
		fclose(bin_file);
		brres_free(&result);
		return 1;
	}

	fclose(bin_file);
	brres_free(&result);

	return 0;
}

```
