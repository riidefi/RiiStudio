[![crates.io](https://img.shields.io/crates/v/szs.svg)](https://crates.io/crates/szs)
[![docs.rs](https://docs.rs/szs/badge.svg)](https://docs.rs/szs/)

# `szs`
Lightweight crate for SZS (\"Yaz0\") compression/decompression used in the Nintendo GameCube/Wii games. The library provides C, C++ and C# bindings. YAY0 ("SZP") is supported, too.

### Rust
The following snippet demonstrates how to compress a file as a SZS format using Rust:

```rs
// Sample source bytes to be encoded.
let src_data: Vec<u8> = "Hello, World!".as_bytes().to_vec();

match szs::encode(&src_data, szs::EncodeAlgo::Nintendo) {
    Ok(encoded_data) => {
        println!("Encoded into {} bytes", encoded_data.len());
    }
    Err(szs::EncodeAlgoError::Error(err_msg)) => {
        println!("Encoding failed: {}", err_msg);
    }
}
```
And similarly, to decompress:
```rs
match szs::decode(&encoded_data) {
    Ok(decoded_data) => {
        println!("Decoded {} bytes", decoded_data.len());
    }
    Err(szs::EncodeAlgoError::Error(err_msg)) => {
        println!("Decoding failed: {}", err_msg);
    }
}
```

## C# Bindings
The following C# bindings are [provided](https://github.com/riidefi/RiiStudio/tree/master/source/szs/c%23):
```cs
using System;

public class SZSExample
{
    public static void Main(string[] args)
    {
        byte[] data = ...; // Initialize your data here

        szs.CompressionAlgorithm algorithm = szs.CompressionAlgorithm.Nintendo;
        byte[] encodedData = szs.Encode(data, algorithm);

        if (encodedData == null || encodedData.Length == 0)
        {
            Console.WriteLine("Failed to compress.");
            return;
        }

        Console.WriteLine($"Encoded {encodedData.Length} bytes.");
    }
}
```

##### Warning: These algorithms are currently implemented in **the C programming language**, and not in Rust. While they have been rigorously validated, please use at your own risk. A Rust rewrite is planned.

### Algorithms

| Algorithm (`rszst compress --algorithm` form) | Rust form | C bindings                     | Desc       |
|-----------------------------------------------|-----------|--------------------------------|------------|
| `nintendo`            | `EncodeAlgo::Nintendo`            | `RII_SZS_ENCODE_ALGO_NINTENDO` | Boyer-moore-horspool (Reverse engineered. 1:1 matching source files--relevant for decompilation projects)
| `mk8`                 | `EncodeAlgo::Mk8`                 | `RII_SZS_ENCODE_ALGO_MK8`      | MK8 compressor (Reverse engineered. Credit @aboood40091)
| `mkw-sp`              | `EncodeAlgo::MkwSp`               | `RII_SZS_ENCODE_ALGO_MKWSP`    | MKW-SP
| `ctgp`                | `EncodeAlgo::CTGP`                | `RII_SZS_ENCODE_ALGO_CTGP`     | CTGP (Reverse engineered. 1:1 matching)
| `worst-case-encoding` | `EncodeAlgo::WorstCaseEncoding`   | `RII_SZS_ENCODE_ALGO_WORST_CASE_ENCODING` | Worst case
| `haroohie`            | `EncodeAlgo::Haroohie`            | `RII_SZS_ENCODE_ALGO_HAROOHIE`| Haroohie (credit @Gericom, adapted from MarioKartToolbox)
| `ct-lib`              | `EncodeAlgo::CTLib`               | `RII_SZS_ENCODE_ALGO_CTLIB`   | CTLib (credit @narahiero, adapted from CTLib)
| `lib-yaz0`            | `EncodeAlgo::LibYaz0`             | `RII_SZS_ENCODE_ALGO_LIBYAZ0` | libyaz0 (Based on wszst. credit @aboood40091)

Generally, the `mk8` algorithm gets acceptable compression the fastest. For cases where filesize matters, `lib-yaz0` ties `wszst ultra` for the smallest filesizes, while being ~25% faster.


### Large file comparison
NSMBU 8-43 (63.9 MB decompressed)
| Method               | Time (Avg 3 runs) | Compression Rate | File Size |
|----------------------|-------------------|------------------|-----------|
| lib-yaz0             |            25.97s |           29.32% |  18.74 MB |
| mkw                  |            78.26s |           29.40% |  18.79 MB |
| mkw-sp               |            49.28s |           29.74% |  19.01 MB |
| haroohie             |            11.44s |           29.74% |  19.01 MB |
| ct-lib               |             5.32s |           29.74% |  19.01 MB |
| mk8                  |             1.46s |           30.12% |  19.25 MB |
| ctgp                 |            12.05s |           40.91% |  26.14 MB |
| worst-case-encoding  |             0.07s |          112.50% |  71.90 MB |

*\* Average of 3 runs; x64 Clang (15, 16) build tested on an Intel i7-9750H on Windows 11*

Generally, the `mk8` algorithm gets acceptable compression the fastest. For cases where filesize matters, `lib-yaz0` ties `wszst ultra` for the smallest filesizes, while being ~25% faster.

### Small file comparison
**Task: Compress N64 Bowser Castle** (Source filesize: 2,574,368)
| Method              | Time Taken             | Compression Rate |
|---------------------|------------------------|------------------|
| lib-yaz0            | 2.03s                  |       **56.65%** |
| nintendo            | 5.93s                  |           56.87% |
| mkw-sp              | 3.76s                  |           57.23% |
| Haroohie            | 0.58s                  |           57.23% |
| CTLib               | 0.32s                  |           57.24% |
| MK8                 | 0.09s                  |           57.59% |
| ctgp                | 0.31s                  |           71.41% |
| worst-case-encoding | **0s**                 |          112.50% |
| **Comparison with other libraries:** |       |                  |
| Haroohie (C#)       | 0.71s                  |           57.23% |
| wszst (fast)        | **0.387s** (via shell) |           65.78% |
| wszst (standard)    | 1.776s (via shell)     |           57.23% |
| wszst (ultra)       | 2.727s (via shell)     |       **56.65%** |
| yaz0-rs             | 11.34s (via shell)     |           56.87% |

*\* Average of 3 runs; x64 Clang (15, 16) build tested on an Intel i7-9750H on Windows 11*


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
#### [A CMake example is provided, too.](https://github.com/riidefi/RiiStudio/tree/master/source/szs/c%2b%2b)
```cpp
#include `szs.h`

// Boyer-Moore-horspool variant
szs::Algo algorithm = szs::Algo::Nintendo;
auto encoded = szs::encode(data, algorithm);
if (!encoded)
	std::println(stderr, "Failed to compress: {}.", encoded.error()); {
	return -1;
}
std::vector<u8> szs_data = *encoded;
std::println("Encoded {} bytes.", szs_data.size());
```

#### License
This library is published under the MIT license.
