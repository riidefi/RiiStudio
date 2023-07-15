# avir-rs
avir-rs: Rust bindings for avir, a SIMD image resizing / scaling library.

## Usage 

### Rust
The following snippet demonstrates how to downscale an image using a fast Lanczos algorithm:

```rust
let mut dst = vec![0; dst_len];
let src = vec![0; src_len];
librii::clancir_resize(&mut dst, width / 2, height / 2, &src, width, height);
```
### C/C++
The library provides C bindings, making it useful in both Rust and C/C++ based projects.
```cpp
#include "avir_rs.h"

unsigned char dst[dst_len];
unsigned char src[src_len];
clancir_resize(dst, sizeof(dst), width / 2, height / 2, src, sizeof(src), width, height);
```
The relevant header is available in `include/avir_rs.h`.

#### License
This library is published under MIT.

Uses [avir](https://github.com/avaneev/avir).
