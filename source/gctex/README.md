# gctex
gctex is a Rust crate designed for encoding and decoding texture formats used in the Nintendo GameCube and Wii games. The library provides C bindings, making it useful in both Rust and C/C++ based projects.

## Usage 

### Rust
The following snippet demonstrates how to encode a texture in CMPR format using Rust:

```rust
let mut dst = vec![0; dst_len];
let src = vec![0; src_len];
gctex::rii_encode_cmpr(&mut dst, &src, width, height);
```

### C# Bindings
See https://github.com/riidefi/RiiStudio/tree/master/source/gctex/examples/c%23
```cs
byte[] dst = new byte[dst_len];
byte[] src = new byte[src_len];
gctex.Encode(0xE /* CMPR */, dst, src, width, height);
```

### C/C++
For C/C++ applications, you can use the following syntax:
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
| CMPR    | WSZST           | Dolphin Emulator |
| I4      | Builtin         | Dolphin Emulator (SIMD) |
| I8      | Builtin         | Dolphin Emulator (SIMD) |
| IA4     | Builtin         | Dolphin Emulator |
| IA8     | Builtin         | Dolphin Emulator (SIMD) |
| RGB565  | Builtin         | Dolphin Emulator |
| RGB5A3  | Builtin         | Dolphin Emulator (SIMD) |
| RGBA8   | Builtin         | Dolphin Emulator (SIMD) |

Please note, SIMD texture decoding for I4, I8 and IA8 formats uses SSE3 instructions with a fallback to SSE2 if necessary, and these are implemented based on the Dolphin Emulator's texture decoding logic.

#### License
This dynamically linked library is published under GPLv2.
