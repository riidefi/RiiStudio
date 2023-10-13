# szs
szs is a WIP crate for compressing and decompressing SZS files (Yaz0 encoding) used in the Nintendo GameCube and Wii games. The library provides C bindings, making it useful in both Rust and C/C++ based projects.

##### Warning: The library is not currently in a fully functional state. Use at your own risk. 0.1.0 will start the release series.
##### Warning: These algorithms are currently implemented in **the C programming language**, and not in Rust. While they have been rigorously validated, please use at your own risk. A Rust rewrite is planned.

### Algorithms
- Boyer-moore-horspool (Reverse engineered. 1:1 matching source files--relevant for decompilation projects)
- MK8 compressor (Reverse engineered. Credit @aboood40091)
- MKW-SP
- CTGP (Reverse engineered. 1:1 matching)
- Worst case
- Haroohie (credit @Gericom, adapted from MarioKartToolbox)
- CTLib (credit @narahiero, adapted from CTLib)
- libyaz0 (Based on wszst. credit @aboood40091)

### Stats
**Task: Compress N64 Bowser Castle** (Source filesize: 2,574,368)
| Method | Time Taken | Compression Rate |
|--------|------------|------------------|
| worst-case-encoding | **0s** | 112.50% |
| MK8  | 0.09s | 57.59% |
| ctgp | 0.31s | 71.41% |
| CTLib | 0.32s | 57.24% |
| Haroohie | 0.58s | 57.23% |
| lib-yaz0 | 2.03s | **56.65%** |
| mkw-sp | 3.76s | 57.23% |
| nintendo | 5.93s | 56.87% |
| **Comparison with other libraries:** | | |
| Haroohie (C#) | 0.71s | 57.23% |
| wszst (fast) | **0.387s** (via shell) | 65.78% |
| wszst (standard) | 1.776s (via shell) | 57.23% |
| wszst (ultra) | 2.727s (via shell) | **56.65%** |
| yaz0-rs | 11.34s (via shell) | 56.87% |

*\* Average of 3 runs; x64 Clang (15, 16) build tested on an Intel i7-9750H on Windows 11*

Generally, the `CTLib` algorithm gets acceptable compression the fastest. For cases where filesize matters, `lib-yaz0` ties `wszst ultra` for the smallest filesizes, while being ~25% faster. For absolute speed, the `mk8` algorithm achieves the best results.

#### Large file comparison
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

#### License
This library is published under the MIT license.
