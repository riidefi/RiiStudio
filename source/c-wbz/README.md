# c-wbz
C/C++ bindings for [wbz_converter](https://github.com/GnomedDev/wbz-to-szs-rs)

## Example (Decoding)
```cpp
#include "wbz_rs.h"

std::string_view autoadd_path = "path/to/your/autoadd";
auto result = wbz_rs::decode_wbz(data, autoadd_path);
if (!result) {
	std::println(stderr, "Failed to decode WBZ file: {})", wbz_rs::error_to_string(result.error()));
	return;
}
std::vector<uint8_t> decoded = *result;
std::println("WBZ decoded successfully: {}", decoded.size());
```

## Example (Encoding)
```cpp
#include "wbz_rs.h"

std::string_view autoadd_path = "path/to/your/autoadd";
auto result = wbz_rs::encode_wbz(data, autoadd_path);
if (!result) {
	std::println(stderr, "Failed to encode WBZ file: {})", wbz_rs::error_to_string(result.error()));
	return;
}
std::vector<uint8_t> encoded = *result;
std::println("WBZ encoded successfully: {}", encoded.size());
```

## Example (C API / Advanced usage)
```c
#include "wbz_rs.h"

const char autoadd_path[] = "path/to/your/autoadd";
// In-place conversion of U8 -> WU8 before compression
wbzrs_buffer wbz_file = {};
int32_t stat = wbzrs_encode_wbz_inplace(u8_buffer, sizeof(u8_buffer), autoadd_path, &wbz_file);
if (stat != WBZ_ERR_OK) {
	fprintf(stderr, "Failed to encode WBZ file: %s\n", wbzrs_error_to_string(stat));
	wbzrs_free_buffer(&wbz_file);
	return -1;
}
printf("WBZ encoded successfully: %u bytes\n", wbz_file.size);
// Remember to free the buffer when done
wbzrs_free_buffer(&wbz_file);
```
