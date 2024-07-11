[![crates.io](https://img.shields.io/crates/v/gctex.svg)](https://crates.io/crates/gctex)
[![docs.rs](https://docs.rs/gctex/badge.svg)](https://docs.rs/gctex/)

# `gctex`
gctex is a Rust crate designed for encoding and decoding texture formats used in the Nintendo GameCube and Wii games. The library provides C bindings, making it useful in both Rust and C/C++ based projects.

## Usage 

### Rust
The following snippet demonstrates how to encode a texture in CMPR format using Rust:

```rust
let width: u32 = 32;
let height: u32 = 32;
let src = vec![0; (width*height*4) as usize];
let dst = gctex::encode(gctex::TextureFormat::CMPR, &src, width, height);
```

### C# Bindings
See https://github.com/riidefi/RiiStudio/tree/master/source/gctex/examples/c%23
```cs
byte[] dst = new byte[dst_len];
byte[] src = new byte[src_len];
gctex.Encode(0xE /* CMPR */, dst, src, width, height);
```

### C/C++
See https://github.com/riidefi/RiiStudio/tree/master/source/gctex/examples/c%2b%2b
```cpp
#include "gctex.h"

unsigned char dst[dst_len];
unsigned char src[src_len];
rii_encode_cmpr(dst, sizeof(dst), src, sizeof(src), width, height);
```
The relevant header is available in `include/gctex.h`.

#### Supported Formats
All supported texture formats and their respective encoding and decoding sources are listed below.

| Format  | Encoding Source | Decoding Source |
|---------|-----------------|-----------------|
| CMPR    | Builtin         | Dolphin Emulator (SIMD) / Rust non-SIMD fallback |
| I4      | Builtin         | Builtin (SIMD (SSE3)) |
| I8      | Builtin         | Dolphin Emulator (SIMD) / Rust non-SIMD fallback |
| IA4     | Builtin         | Builtin |
| IA8     | Builtin         | Dolphin Emulator (SIMD) / Rust non-SIMD fallback |
| RGB565  | Builtin         | Dolphin Emulator (SIMD) / Rust non-SIMD fallback |
| RGB5A3  | Builtin         | Dolphin Emulator (SIMD) / Rust non-SIMD fallback |
| RGBA8   | Builtin         | Builtin (SIMD (SSE3)) |
| C4      | -               | Dolphin Emulator / No fallback |
| C8      | -               | Dolphin Emulator / No fallback |
| C14     | -               | Dolphin Emulator / No fallback |


Please note, SIMD texture decoding for I4, I8 and IA8 formats uses SSE3 instructions with a fallback to SSE2 if necessary (excepting I4), and these are implemented based on the Dolphin Emulator's texture decoding logic.

#### Optional Features
- To avoid needing a C++ compiler or running C++ code, unset the `cpp_fallback` feature to fallback to non-SIMD Rust implementations of I4/I8/IA8/RGB565/RGB5A3 decoding. 
- For debugging the `simd` feature can be disabled to use pure, standard Rust.

#### License
This dynamically linked library is published under GPLv2.
