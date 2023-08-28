# source
An outline of the source tree.

All the folders in the [source](https://github.com/riidefi/RiiStudio/blob/master/source) directory are provided:

| Name                                                                                | Crate? | Description                                        | Programming Language |
|-------------------------------------------------------------------------------------|--------|----------------------------------------------------|----------------------|
| [avir-rs](https://github.com/riidefi/RiiStudio/blob/master/source/avir-rs)          | ✅ | Rust wrapper for a C++ image resizing library.         | Rust/C++             |
| [blender](https://github.com/riidefi/RiiStudio/blob/master/source/blender)          | ❌ | Blender plugin with exporter support for BRRES/BMD.    | Python               |
| [c-discord-rich-presence](https://github.com/riidefi/RiiStudio/blob/master/source/c-discord-rich-presence) | ✅ | C/C++ bindings for the [discord-rich-presence crate](https://github.com/sardonicism-04/discord-rich-presence). Provides an alternative to the official Discord libraries for RPC for C++ applications.                         | Rust                  |
| [c-wbz](https://github.com/riidefi/RiiStudio/blob/master/source/c-wbz)              | ✅ | C/C++ bindings for [wbz_converter](https://github.com/GnomedDev/wbz-to-szs-rs) | Rust |
| [cli](https://github.com/riidefi/RiiStudio/blob/master/source/cli)                  | ❌ | Source for the `rszst` utility.                         | C++, Rust            |
| [core](https://github.com/riidefi/RiiStudio/blob/master/source/core)                | ❌ | Core utilities and structures. Available to all other modules. | C++           |
| [frontend](https://github.com/riidefi/RiiStudio/blob/master/source/frontend)        | ❌ | The main editor itself.                                 | C++                  |
| [gctex](https://github.com/riidefi/RiiStudio/blob/master/source/gctex)              | ✅ | GC/Wii image codec. [Published to crates.io](https://crates.io/crates/gctex)   | Rust/C++          |
| [imcxx](https://github.com/riidefi/RiiStudio/blob/master/source/imcxx)              | ❌ | My C++ wrappers for ImGui.                              | C++                  |
| [LibBadUIFramework](https://github.com/riidefi/RiiStudio/blob/master/source/LibBadUIFramework) | ❌ | Basis of the `plugins` folder, defines some extendable UI structures. |  C++   |
| [librii](https://github.com/riidefi/RiiStudio/blob/master/source/librii)            | ❌ | Library for interacting with Wii data. [Documentation here](https://github.com/riidefi/RiiStudio/blob/master/source/librii/README.md)                 | C++                  |
| [llvm](https://github.com/riidefi/RiiStudio/blob/master/source/llvm)                | ✅ | Rust wrapper for the llvm crash handler.                | Rust/C++             |
| [oishii](https://github.com/riidefi/RiiStudio/blob/master/source/oishii)            | ❌ | My binary IO library.                                   | C++                  |
| [plate](https://github.com/riidefi/RiiStudio/blob/master/source/plate)              | ❌ | Dear ImGui boilerplate code.                            | C++                  |
| [plugins](https://github.com/riidefi/RiiStudio/blob/master/source/plugins)          | ❌ | Specific plugins for the editor: BMD, BRRES, Assimp (to BMD/BRRES). | C++      |
| [rsl](https://github.com/riidefi/RiiStudio/blob/master/source/rsl)                  | ❌ | My standard library: generic template types.            | C++                  |
| [rsmeshopt](https://github.com/riidefi/RiiStudio/blob/master/source/rsmeshopt)      | ❌ | My mesh optimization library.                           | C++                  |
| [rust_bundle](https://github.com/riidefi/RiiStudio/blob/master/source/rust_bundle)  | ✅ | Allows RiiStudio to be `cargo bundle`d into a MacOS .app file. | Rust          |
| [szs](https://github.com/riidefi/RiiStudio/blob/master/source/szs)                  | ✅ | SZS compressing and decompressing algorithms. [Published to crates.io](https://crates.io/crates/szs) | Rust/C++          |
| [tests](https://github.com/riidefi/RiiStudio/blob/master/source/tests)              | ❌ | A CLI tool used by the python unit tests.               | C++                  |
| [updater](https://github.com/riidefi/RiiStudio/blob/master/source/updater)          | ❌ | Application updater                                     | C++                  |
| [vendor](https://github.com/riidefi/RiiStudio/blob/master/source/vendor)            | ❌ | Third-party code.                                       | C, C++               |
| [wiitrig](https://github.com/riidefi/RiiStudio/blob/master/source/wiitrig)          | ✅ | Wii `sin`/`cos` function implementations                | Rust/C++             |

Dependencies between these packages are listed in the following flowchart. Generally, `librii` handles file formats/Wii-specifics while `plugins` provides a higher level interface for the editor. The two main .exe files are `frontend` (RiiStudio.exe) and `cli` (rszst.exe).
```mermaid
flowchart LR

subgraph "Rust Depencies"
	discord-rich-presence
	wbz_converter
    zip-extract
    clap
    reqwest
    simple_logger
end

discord-rich-presence ---> c-discord-rich-presence
wbz_converter --> c-wbz
zip-extract --> riistudio_rs
clap --> rszst_arg_parser
reqwest --> riistudio_rs
simple_logger --> riistudio_rs

subgraph "Rust Crates"
    avir-rs
	c-discord-rich-presence
	c-wbz
	gctex
	wiitrig
	szs
    rszst_arg_parser
    riistudio_rs
end

subgraph "C++ Libraries"
	imcxx
	oishii
	plate
	rsmeshopt
	LibBadUIFramework
end

subgraph "RiiStudio"
    plugins
    updater
	rsl
	librii
    frontend(["frontend (RiiStudio.exe)"])
    cli(["cli (rszst.exe, command-line interface)"])
    tests(["tests (tests.exe, for unit tests)"])
end


%% core --> oishii & plate & rsmeshopt & imcxx & LibBadUIFramework

%% core
%% vendor

%% subgraph "C++ Binaries"
%% 	cli
%% 	tests
%% 	frontend
%% end

rszst_arg_parser --> cli

avir-rs --> librii
c-discord-rich-presence --> rsl
c-wbz --> librii
gctex --> librii
wiitrig --> librii
%%llvm --> cli & frontend & tests
riistudio_rs -->  rsl
szs --> librii

%% core -->  LibBadUIFramework & librii & plugins & rsl & updater & cli & frontend & tests
imcxx --> frontend
LibBadUIFramework --> plugins
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
The following flowchart explains how 3d model data comes in (from Blender, as a .dae/.fbx file) and eventually ends up as a .brres or .bmd file (with model optimizations).
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
