# librii

## File Format Libraries

### assimp
Modern C++ bindings for the Assimp library

### assimp2rhst
Converts an assimp scene to a RHST intermediate file

### crate
BRRES material/animation preset library.

### egg
EGG posteffect support:
- .bdof/.pdof
- .bfg*
- .blight/.plight
- .blmap/.plmap
- .bblm/.pblm

### librii::g3d
BRRES support.

#### 1. Testing
Every BRRES format is validated to produce 1:1 byte-identical output, even the Intermediate Representations (see below). Tests can be found in the `tests` folder.

#### Core implementation moved into `brres` crate

See internal docs [here](../brres/lib/brres-sys/README.md).

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

### librii::tev
TEV fixed-function pixel shader expression solver and potentially more.

## System/Hardware Libraries

### librii::nitro
Support for DS fixed-point types.

### librii::gpu
Allows direct interaction with the Wii GPU.

## Generic Libraries

### librii::math
3D math helpers: AABB, SRT3

