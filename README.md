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
