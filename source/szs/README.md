# szs
szs is a WIP crate for compressing and decompressing SZS files (Yaz0 encoding) used in the Nintendo GameCube and Wii games. The library provides C bindings, making it useful in both Rust and C/C++ based projects.

##### Warning: The library is not currently in a fully functional state. Use at your own risk. 0.1.0 will start the release series.
##### Warning: These algorithms are currently implemented in **the C programming language**, and not in Rust. While they have been rigorously validated, please use at your own risk. A Rust rewrite is planned.

### Algorithms
- Boyer-moore-horspool (Reverse engineered. 1:1 matching source files--relevant for decompilation projects)
- SP
- CTGP (Reverse engineered. 1:1 matching)
- Worst case
- Haroohie (credit @Gericom, adapted from MarioKartToolbox)
- CTLib (credit @narahiero, adapted from CTLib)

### Stats
**Task: Compress N64 Bowser Castle** (Source filesize: 2,574,368)
| Method | Time Taken | Compression Rate |
|--------|------------|------------------|
| worst-case-encoding | **0s** | 112.50% |
| ctgp | 0.31s | 71.41% |
| CTLib | 0.32s | 57.24% |
| Haroohie | 0.58s | 57.23% |
| mkw-sp | 3.76s | 57.23% |
| nintendo | 5.93s | **56.87%** |
| **Comparison with other libraries:** | | |
| Haroohie (C#) | 0.71s | 57.23% |
| wszst (fast) | **0.387s** (via shell) | 65.78% |
| wszst (standard) | 1.776s (via shell) | 57.23% |
| wszst (ultra) | 2.727s (via shell) | **56.65%** |
| yaz0-rs | 11.34s (via shell) | 56.87% |

*\* Average of 3 runs; x64 Clang (15, 16) build tested on an Intel i7-9750H on Windows 11*

In most cases, the `CTLib` algorithm gets the best compression the fastest, with marginally better results from the `Haroohie` algorithm a bit slower. `wszst ultra` gets the smallest filesizes.

### Rust
The following snippet demonstrates how to compress a file as a SZS format using Rust:

```rs
// Sample source bytes to be encoded.
let src_data: Vec<u8> = "Hello, World!".as_bytes().to_vec();

// Calculate the upper bound for encoding.
let max_len = encoded_upper_bound(src_data.len() as u32);

// Allocate a buffer based on the calculated upper bound.
let mut dst_data: Vec<u8> = vec![0; max_len as usize];

// Boyer-Moore-horspool variant
let algo_number: u32 = 0;

match encode_algo_fast(&mut dst_data, &src_data, algo_number) {
    Ok(encoded_len) => {
        println!("Encoded {} bytes", encoded_len);
        // Optionally: shrink the dst_data to the actual size.
        dst_data.truncate(encoded_len as usize);
    }
    Err(EncodeAlgoError::Error(err_msg)) => {
        println!("Encoding failed: {}", err_msg);
    }
}
```

### Example (C Bindings)
```c
#include `szs.h`

// Calculate the upper bound for encoding.
u32 max_size = riiszs_encoded_upper_bound(sizeof(data));

// Allocate a buffer based on the calculated upper bound.
void* encoded_buf = malloc(max_size);
if (!buf) {
	fprintf(stderr, "Failed to allocate %u bytes.\n", max_size);
	return -1;
}

// Boyer-Moore-horspool variant
u32 algorithm = RII_SZS_ENCODE_ALGO_NINTENDO;

u32 actual_len = 0;
const char* ec = riiszs_encode_algo_fast(encoded_buf, max_size, data, sizeof(data), &actual_len, algorithm);
if (ec != NULL) {
	fprintf(stderr, "Failed to compress: %s\n", ec);
	riiszs_free_error_message(ec);
	return -1;
}
printf("Encoded %u bytes.\n", actual_len);
// Optionally: shrink the dst_data to the actual size.
encoded_buf = realloc(encoded_buf, actual_len);
```

### C++ Wrapper on top of C Bindings
```cpp
#include `szs.h`

// Boyer-Moore-horspool variant
szs::Algo algorithm = szs::Algo::Nintendo;
auto encoded = szs::encode_algo(data, algorithm);
if (!encoded)
	std::println(stderr, "Failed to compress: {}.", encoded.error()); {
	return -1;
}
std::vector<u8> szs_data = *encoded;
std::println("Encoded {} bytes.", szs_data.size());
```

#### License
This library is published under the MIT license.
