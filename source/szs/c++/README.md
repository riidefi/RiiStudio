# C/C++ Bindings

C/C++ bindings are provided for the native library via "szs.h". The following example demonstrates integration with CMake.

### Requirements
- You must have rust installed.

### Caveats
Because the library itself has C++ code in it, to avoid needing to sync some settings, using a .dll is by far the easiest approach, which is demonstrated in the example. Static linking is possible, though is typically only useful for release builds. RiiStudio uses the following setup
```cmake
# In Debug builds, we are forced to use .dll due to incompatible _ITERATOR_DEBUG_LEVEL values: 0 in cc-rs via Rust and 2 in C++.
# In release builds we should prefer static libraries, though, to minimize failure points during the update process.
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(RECURSIVE_CPP_CRATE_TYPE cdylib)
else()
  set(RECURSIVE_CPP_CRATE_TYPE staticlib)
endif()
corrosion_import_crate(MANIFEST_PATH source/szs/Cargo.toml CRATE_TYPES ${RECURSIVE_CPP_CRATE_TYPE} FLAGS --crate-type=${RECURSIVE_CPP_CRATE_TYPE})
```

## bindings
A library, provides the following API:
```cpp
namespace szs {
  enum class Algo {
    WorstCaseEncoding,
    Nintendo,
    MKWSP,
    CTGP,
    Haroohie,
    CTLib,
    LibYaz0,
    MK8,
  };
  std::string get_version();
  bool is_compressed(std::span<const uint8_t> src);
  uint32_t decoded_size(std::span<const uint8_t> src);
  std::expected<uint32_t, std::string> encode_into(std::span<uint8_t> dst, std::span<const uint8_t> src, Algo algo);
  std::expected<std::vector<uint8_t>, std::string> encode(std::span<const uint8_t> buf, Algo algo);
  std::expected<void, std::string> decode_into(std::span<uint8_t> dst, std::span<const uint8_t> src);
  std::expected<std::vector<uint8_t>, std::string> decode(std::span<const uint8_t> src);
}
```

## simple_example
Example project using the API above:
```cpp
int main() {
  std::string version = szs::get_version();
  std::cout << "szs version: " << version << std::endl;

  std::string sampleText =
      "Hello, world! This is a sample string for testing the szs compression "
      "library."
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHello, world!";

  // Convert string to byte array (UTF-8)
  std::vector<uint8_t> originalData(sampleText.begin(), sampleText.end());

  std::cout << "Original Data:\n";
  std::cout << sampleText << "\n";
  std::cout << "Original Size: " << originalData.size() << " bytes\n\n";

  // Check if data is compressed
  bool isCompressed = szs::is_compressed(originalData);
  std::cout << "Is Original Data Compressed? "
            << (isCompressed ? "true" : "false") << "\n\n";

  // Encode data
  auto maybe_encodedData = szs::encode(originalData, szs::Algo::LibYaz0);
  if (!maybe_encodedData) {
    std::cout << "Failed to encode: " << maybe_encodedData.error() << std::endl;
    return -1;
  }
  std::vector<uint8_t> encodedData = *maybe_encodedData;

  std::cout << "Encoded Size: " << encodedData.size() << " bytes\n\n";

  // Decode data
  auto maybe_decodedData = szs::decode(encodedData);

  if (!maybe_decodedData) {
    std::cout << "Failed to decode: " << maybe_decodedData.error() << std::endl;
    return -1;
  }
  std::vector<uint8_t> decodedData = *maybe_decodedData;

  std::string decodedText((const char*)decodedData.data(), decodedData.size());
  std::cout << "Decoded Data:\n";
  std::cout << decodedText << "\n\n";

  // Check if encoded data is compressed
  bool isEncodedDataCompressed = szs::is_compressed(encodedData);
  std::cout << "Is Encoded Data Compressed? "
            << (isEncodedDataCompressed ? "true" : "false") << "\n\n";

  return 0;
}
```
