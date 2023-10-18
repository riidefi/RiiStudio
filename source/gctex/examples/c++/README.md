# C/C++ Bindings

C/C++ bindings are provided for the native library via "gctex.h". The following example demonstrates integration with CMake.

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
corrosion_import_crate(MANIFEST_PATH source/rsmeshopt/Cargo.toml CRATE_TYPES ${RECURSIVE_CPP_CRATE_TYPE} FLAGS --crate-type=${RECURSIVE_CPP_CRATE_TYPE})
```

## bindings
A library, provides the following API:
```cpp
namespace gctex {
  //! Get the library version.
  std::string get_version();
  //! Computes the size of a simple image
  uint32_t compute_image_size(uint32_t format,
                              uint32_t width, uint32_t height);
  //! Computes the size of a mipmapped image
  uint32_t compute_image_size_mip(uint32_t format,
                                  uint32_t width, uint32_t height,
                                  uint32_t number_of_images);
  //! Encode from raw 32-bit color to the specified format
  void encode_into(uint32_t format,
                   std::span<uint8_t> dst,
                   std::span<const uint8_t> src,
				   uint32_t width, uint32_t height);
  //! Encode from raw 32-bit color to the specified format
  std::vector<uint8_t> encode(uint32_t format,
                              std::span<const uint8_t> src,
                              uint32_t width, uint32_t height);
  //! Decode from the specified format to raw 32-bit color
  void decode_into(std::span<uint8_t> dst,
                   std::span<const uint8_t> src,
                   uint32_t width, uint32_t height,
                   uint32_t texformat,
                   std::span<const uint8_t> tlut,
                   uint32_t tlutformat);
  //! Decode from the specified format to raw 32-bit color
  std::vector<uint8_t> decode(std::span<const uint8_t> src,
                              uint32_t width, uint32_t height,
                              uint32_t texformat,
                              std::span<const uint8_t> tlut
                              uint32_t tlutformat);
}
```

## simple_example
Example project using the API above:
```cpp
int main() {
  std::string version = gctex::get_version();
  std::cout << "gctex version: " << version << std::endl;

  uint32_t width = 2;
  uint32_t height = 2;
  std::array<uint8_t, 4 * 4> square{
      0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
  };

  // Encode as CMPR
  std::vector<uint8_t> encoded =
      gctex::encode(TEXFMT_CMPR, square, width, height);

  // Decode back from CMPR
  std::vector<uint8_t> decoded =
      gctex::decode(encoded, width, height, TEXFMT_CMPR, {}, 0);

  return 0;
}
```
