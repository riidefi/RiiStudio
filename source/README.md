# source
An outline of the source tree.

```mermaid
flowchart LR

subgraph "Rust Depencies"
	discord-rich-presence
	wbz_converter
    zip-extract
    clap
    curl
    simple_logger
end

discord-rich-presence ---> c-discord-rich-presence
wbz_converter --> c-wbz
zip-extract --> rust
clap --> rust
curl --> rust
simple_logger --> rust

subgraph "Rust Crates"
	llvm
    avir-rs
	c-discord-rich-presence
	c-wbz
	dolphin-memory-engine-rs
	gctex
	rust
end

subgraph "C++ Libraries"
	imcxx
	oishii
	plate
	rsmeshopt
	LibBadUIFramework
	librii
	rsl
end

subgraph "RiiStudio"
    updater
    plugins
    frontend(["frontend (RiiStudio.exe)"])
    cli(["cli (rszst.exe, command-line interface)"])
    tests(["tests (tests.exe, for unit tests)"])
end

core --> oishii & plate & rsmeshopt & imcxx & LibBadUIFramework

%% core
%% vendor

%% subgraph "C++ Binaries"
%% 	cli
%% 	tests
%% 	frontend
%% end

avir-rs --> librii
c-discord-rich-presence --> rsl
c-wbz --> librii
dolphin-memory-engine-rs --> frontend
gctex --> librii
llvm --> cli & frontend & tests
rust -->  rsl & cli & frontend & tests

%% core -->  LibBadUIFramework & librii & plugins & rsl & updater & cli & frontend & tests
imcxx --> frontend
LibBadUIFramework --> plugins & librii
%%  & frontend
librii --> cli & frontend & tests
oishii --> rsl & librii
plate --> frontend
plugins --> frontend
rsl --> librii & plugins & updater
%% & cli & frontend & tests
rsmeshopt --> librii
updater --> frontend
%% vendor --> LibBadUIFramework & librii & plugins & rsl & updater & cli & frontend & tests
```

### Data pipeline for model creation
```mermaid

flowchart LR

subgraph "3D Software"
Blender
Maya
end

Blender --> dae & fbx

Maya --> fbx

subgraph Interchange formats
dae
fbx
.json
end
subgraph Assimp library
Assimp
end
subgraph "RHST Structure (C++)"
RHST
end
subgraph Game files
BMD
BRRES
end
Blender --> |"Blender Plugin (python)"|.json

.json --> |librii\rhst\RHST.cpp| RHST

RHST --> |"plugins\RHSTImporter.cpp"| BMD
RHST --> |"plugins\RHSTImporter.cpp"| BRRES

Assimp --> |"librii\assimp2rhst\*.cpp"| RHST
dae & fbx --> Assimp


RHST --> RHSTOptimizer
RHSTOptimizer["Intermediate format optimizer\n\n(librii\rhst\RHSTOptimizer.cpp)"] --> RHST

RHSTOptimizer <--> rsmeshopt
rsmeshopt["rsmeshopt\n\n(Mesh optimization library)"] <--> draco & meshoptimizer & tristrip & TriStripper & TriFanMeshOptimizer & HaroohieTriStripifier
```

## avir-rs
Rust wrapper for a C++ image resizing library.

## blender
Blender plugin with exporter support for BRRES/BMD.

## cli
Source for the `rszst` utility.

## core
Core utilities and structures. Available to all other modules.

## frontend
The main editor itself.

## gctex
GC/Wii image codec. https://crates.io/crates/gctex

## imcxx
My C++ wrappers for ImGui.

## LibBadUIFramework
Basis of the `plugins` folder, defines some extendable UI structures.

## librii
Library for interacting with Wii data.

## llvm
Rust wrapper for the llvm crash handler.

## oishii
My binary IO library.

## plate
Dear ImGui boilerplate code.

## plugins
Specific plugins for the editor.
- BMD
- BRRES
- KMP
- BFG
- BLIGHT
- BLMAP
- Assimp (to BMD/BRRES)

## rsl
My standard library: generic template types.

## rsmeshopt
My mesh optimization library.

## rust
Rust code other modules can depend on

## rust_bundle
Allows RiiStudio to be `cargo bundle`d into a MacOS .app file.

## scripts
Once housed scripts for generating format Node files. Now just has a CMAKE config for targetting the Wii itself.

## tests
A CLI tool used by the python unit tests.

## updater
Application updater

## vendor
Third-party code.
