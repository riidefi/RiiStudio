# brres-sys

Implements a Rust layer on top of `librii::g3d`'s JSON export-import layer. Importantly, large buffers like texture data and vertex data are not actually encoded in JSON but passed directly as a binary blob. This allows JSON files to stay light.

Exposes the following Rust interface
```rs
pub struct CBrresWrapper<'a> {
    pub json_metadata: &'a str,
    pub buffer_data: &'a [u8],
    // ...
}

impl<'a> CBrresWrapper<'a> {
	// .bin -> .json
    pub fn from_bytes(buf: &[u8]) -> anyhow::Result<Self>;
    // .json -> .bin
    pub fn write_bytes(json: &str, buffer: &[u8]) -> anyhow::Result<Self>;
}
```

This wraps the following C interface, which can still be used (say for other language bindings).
```c
// include/brres_sys.h
struct CResult {
	const char* json_metadata;
	uint32_t len_json_metadata;
	const void* buffer_data;
	uint32_t len_buffer_data;
	void (*freeResult)(struct CResult* self);
	void* opaque;
};
uint32_t brres_read_from_bytes(CResult* result, const void* buf,
                               uint32_t len);
uint32_t brres_write_bytes(CResult* result, const char* json,
                          uint32_t json_len, const void* buffer,
                          uint32_t buffer_len);
void brres_free(CResult* result);
```
e.g. the following idiomatic C code:
```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "include/brres_sys.h"

int main() {
	uint8_t my_brres_file[] = { /* ... */ }; // Initialize with actual .brres file data

	CResult result = {};
	uint32_t ok = brres_read_from_bytes(&result, my_brres_file, sizeof(my_brres_file));

	if (!ok) {
		printf("Failed to read .brres: %*.s\n", (size_t)result.len_json_metadata, result.json_metadata);
		brres_free(&result); // Free the error message
		return 1;
	}

	// Write JSON metadata to a file
	FILE *json_file = fopen("output.json", "wb");
	if (json_file == NULL) {
		perror("Failed to open output.json for writing");
		brres_free(&result);
		return 1;
	}

	size_t written = fwrite(result.json_metadata, 1, result.len_json_metadata, json_file);
	if (written != result.len_json_metadata) {
		perror("Failed to write all JSON metadata to output.json");
		fclose(json_file);
		brres_free(&result);
		return 1;
	}

	fclose(json_file);

	// Write binary buffer data to a file
	FILE *bin_file = fopen("output.bin", "wb");
	if (bin_file == NULL) {
		perror("Failed to open output.bin for writing");
		brres_free(&result);
		return 1;
	}

	written = fwrite(result.buffer_data, 1, result.len_buffer_data, bin_file);
	if (written != result.len_buffer_data) {
		perror("Failed to write all buffer data to output.bin");
		fclose(bin_file);
		brres_free(&result);
		return 1;
	}

	fclose(bin_file);
	brres_free(&result);

	return 0;
}

```
