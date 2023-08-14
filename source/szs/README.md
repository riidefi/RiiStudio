# szs
szs is a WIP crate for compressing and decompressing SZS formats used in the Nintendo GameCube and Wii games. The library provides C bindings, making it useful in both Rust and C/C++ based projects.

Warning: The library is not currently in a fully functional state. Use at your own risk. 0.1.0 will start the release series.

#### Algorithms
- Booyer-moore-horspool (1:1 matching source files--relevant for decompilation projects), Brute force, CTGP (1:1 matching), Fast

### Rust
The following snippet demonstrates how to compress a file as a SZS format using Rust:

```rs
// Sample source bytes to be encoded.
let src_data: Vec<u8> = "Hello, World!".as_bytes().to_vec();

// Calculate the upper bound for encoding.
let max_len = encoded_upper_bound(src_data.len() as u32);

// Allocate a buffer based on the calculated upper bound.
let mut dst_data: Vec<u8> = vec![0; max_len as usize];

// Boyer-Moore-hoorspool variant
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

### Example (C++ Bindings)
```cpp
#include `szs.h`

// Boyer-Moore-hoorspool variant
szs::Algo algorithm = szs::Algo::Nintendo;
auto encoded = szs::encode_algo(data, algorithm);
if (!encoded)
	std::println(stderr, "Failed to compress: {}.", encoded.error()); {
	return -1;
}
std::vector<u8> szs_data = *encoded;
std::println("Encoded {} bytes.", szs_data.size());
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

// Boyer-Moore-hoorspool variant
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
 
#### License
This library is published under the MIT license.