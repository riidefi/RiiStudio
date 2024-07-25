include(FetchContent)

# Install Corrosion
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG        v0.4.4 # Necessary for offline builds
)
FetchContent_MakeAvailable(Corrosion)
# Install szs
FetchContent_Declare(
  szs
  URL      https://crates.io/api/v1/crates/szs/0.3.7/download
  URL_HASH MD5=F5A78B848263F7F5B1DF989D770C9F87 # Necessary for offline builds
)
FetchContent_MakeAvailable(szs)

# In Debug builds, we are forced to use .dll due to incompatible _ITERATOR_DEBUG_LEVEL values: 0 in cc-rs via Rust and 2 in C++.
# In release builds we should prefer static libraries, though, due to minimize failure points during the update process.
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(RECURSIVE_CPP_CRATE_TYPE cdylib)
#else()
#  set(RECURSIVE_CPP_CRATE_TYPE staticlib)
#endif()

corrosion_import_crate(
  MANIFEST_PATH ${szs_SOURCE_DIR}/Cargo.toml
  CRATE_TYPES ${RECURSIVE_CPP_CRATE_TYPE}
  FLAGS --crate-type=${RECURSIVE_CPP_CRATE_TYPE}
)
target_include_directories(szs INTERFACE ${szs_SOURCE_DIR}/include)
