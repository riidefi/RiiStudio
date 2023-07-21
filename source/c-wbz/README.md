# c-wbz
C/C++ bindings for wbz_converter

## Example
```cpp
#include "wbz_rs.h"

std::string_view autoadd_path = "path/to/your/autoadd";
auto result = wbz_rs::decode_wbz(data, autoadd_path);
if (!result) {
	std::cout << "Failed to decode WBZ file: " << wbz_rs::error_to_string(result.error()) << '\n';
	return;
}
std::vector<uint8_t> decoded = *result;
std::cout << "WBZ decoded successfully: " << decoded.size() << " bytes\n";
```
