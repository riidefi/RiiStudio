# C/C++ Bindings

C/C++ bindings are provided for the native library via "rsmeshopt.h". The following example demonstrates integration with CMake.

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
namespace rsmeshopt {
  enum class StripifyAlgo {
    NvTriStripPort = 0,
    TriStripper,
    MeshOpt,
    Draco,
    DracoDegen,
    Haroohie,
  };
  std::expected<std::vector<uint32_t>, std::string>
  DoStripifyAlgo_(StripifyAlgo algo,
                  std::span<const uint32_t> index_data,
                  std::span<const vec3> vertex_data,
                  uint32_t restart = ~0u);
  std::expected<std::vector<uint32_t>, std::string>
  MakeFans_(std::span<const uint32_t> index_data,
            uint32_t restart,
            uint32_t min_len,
            uint32_t max_runs);
}
```

## simple_example
Example project using the API above:
```cpp
int main() {
  std::string version = rsmeshopt::get_version();
  std::cout << "rsmeshopt version: " << version << std::endl;

  // 0---1
  // |  /|
  // | / |
  // 2---3

  std::array<uint32_t, 6> square{
      2, 1, 0, // Triangle 1
      1, 2, 3, // Triangle 2
  };

  // Triangle strip
  auto stripped_maybe = rsmeshopt::DoStripifyAlgo_(
      rsmeshopt::StripifyAlgo::NvTriStripPort, square, {});
  if (!stripped_maybe) {
    std::cout << "Failed to strip: " << stripped_maybe.error() << std::endl;
    return -1;
  }
  std::vector<uint32_t> stripped = *stripped_maybe;
  std::cout << "Stripped: ";
  for (auto val : stripped) {
    std::cout << val << " ";
  }
  std::cout << std::endl;

  return 0;
}
```
