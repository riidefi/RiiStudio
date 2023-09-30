# RiiStudio
Editor for various 3D model formats

![Importing](https://raw.githubusercontent.com/riidefi/RiiStudio/gh-pages/assets/alpha_3.0/import_brres.gif)

## Editable File Formats
| Format | Can Open | Can Save |
|--------|----------|----------|
| BMD    | Yes      | Yes      |
| BDL    | Yes      | As .BMD  |
| BRRES* | Yes      | Yes      |
| KMP    | Yes      | Yes      |
| .bblm (bloom) | Yes      | Yes      |
| .bdof (Depth of Field) | Yes      | Yes      |
| .bfg (fog) | Yes      | Yes      |
| .blight (lighting) | Yes      | Yes      |
| .blmap (matcaps) | Yes      | Yes      |
| .btk | WIP  | No |

\* MDL0, TEX0, SRT0, CHR0, CLR0, PAT0

## Importing
Assimp supported formats like FBX and DAE can be imported as BMD/BRRES*.

\* Currently limited to non-rigged models\

## RiiStudio (as a C++ library) also supports
| Format | Can Open | Can Save |
|--------|----------|----------|
| U8     | Yes      | Yes      |
| SZS    | Yes      | Yes      |

## Overview of source tree
[An overview of the source code is available here](https://github.com/riidefi/RiiStudio/blob/master/source/README.md)

## Command Line Interface
```
RiiStudio CLI Alpha 5.10.13 (Hotfix 4) (Built Aug 23 2023 at 21:50:26, Clang 16.0.5)
Usage: rszst.exe <COMMAND>

Commands:
  import-brres      Import a .dae/.fbx file as .brres
  import-bmd        Import a .dae/.fbx file as .bmd
  decompress        Decompress a .szs file
  compress          Compress a file as .szs
  rhst2-brres       Convert a .rhst file to a .brres file
  rhst2-bmd         Convert a .rhst file to a .bmd file
  extract           Extract a .szs file to a folder
  create            Create a .szs file from a folder
  kmp-to-json       Dump a kmp as json
  json-to-kmp       Convert to kmp from json
  kcl-to-json       Dump a kcl as json
  dump-presets      Dump presets of a model to a folder
  optimize          Optimize a BRRES or BMD file
  help              Print this message or the help of the given subcommand(s)

Options:
  -h, --help     Print help
  -V, --version  Print version
```

Example: Optimizing an existing .brres file with default settings
```
rszst.exe optimize file.brres
```
Example: Creating a .bmd file from a .dae file
```
rszst.exe import-bmd cube.dae
```
Example: Dumping all material/animation presets of a model to a folder
```
rszst dump-presets course_model.brres my_preset_folder
```
Example: Converting a binary .kmp file to an editable JSON document
```
rszst kmp-to-json course.kmp course_kmp.json
```
Example: Compressing a YAZ0 file
```
rszst compress file.arc file.szs --algorithm nintendo
```

## Building

### Windows
Open the CMakeLists.txt with Visual Studio, then click "Build".

### Mac
```sh
git clone https://github.com/riidefi/RiiStudio
cd RiiStudio
mkdir build
cd build

brew install cmake assimp glfw freetype llvm
```

| On M1                                                                                | On Intel                                                                                   |
|--------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------|
| `cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/clang++` | `cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++` |

```sh
cmake --build . --config Release --parallel
```

### Building a .app file
```sh
cd source/rust_bundle
cargo bundle --release
```

### Linux
```sh
git clone https://github.com/riidefi/RiiStudio
cd RiiStudio
mkdir build
cd build

sudo apt-get update --fix-missing
sudo apt install -y cmake mesa-common-dev libglfw3-dev libassimp-dev libfreetype-dev

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel
```

## Translations
### Japanese Translation (日本語)
Credit to @h0d22 on twitter for the translation
https://twitter.com/h0d22

## Credits
 * Dear ImGui - "Bloat-free Immediate Mode Graphical User interface for C++ with minimal dependencies" - https://github.com/ocornut/imgui
 * GLFW, GLM, SDL 
 * Assimp - https://github.com/assimp/assimp - https://github.com/assimp/assimp
 * stb - "stb single-file public domain libraries for C/C++" - https://github.com/nothings/stb
 * Shader generation based on noclip.website's implementation: https://github.com/magcius/noclip.website
 * AVIR - "High-quality pro image resizing / scaling C++ library, image resize" - https://github.com/avaneev/avir
 * The Dolphin Emulator - https://github.com/dolphin-emu/dolphin
 * FontAwesome5 - https://fontawesome.com/
 * IconFontCppHeaders - "C, C++ headers and C# classes for icon fonts" - https://github.com/juliettef/IconFontCppHeaders
 * Portable File Dialogues - "Portable GUI dialogs library, C++11, single-header" - https://github.com/samhocevar/portable-file-dialogs
 * imgui_markdown - Markdown for Dear ImGui - https://github.com/juliettef/imgui_markdown
