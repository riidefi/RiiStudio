include(FetchContent)

# Install Corrosion
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
)
FetchContent_MakeAvailable(Corrosion)
# Install rsmeshopt
FetchContent_Declare(
  rsmeshopt
  URL      https://crates.io/api/v1/crates/rsmeshopt/0.0.9/download
)
FetchContent_MakeAvailable(rsmeshopt)

# In Debug builds, we are forced to use .dll due to incompatible _ITERATOR_DEBUG_LEVEL values: 0 in cc-rs via Rust and 2 in C++.
# In release builds we should prefer static libraries, though, due to minimize failure points during the update process.
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(RECURSIVE_CPP_CRATE_TYPE cdylib)
#else()
#  set(RECURSIVE_CPP_CRATE_TYPE staticlib)
#endif()

corrosion_import_crate(
  MANIFEST_PATH ${rsmeshopt_SOURCE_DIR}/Cargo.toml
  CRATE_TYPES ${RECURSIVE_CPP_CRATE_TYPE}
  FLAGS --crate-type=${RECURSIVE_CPP_CRATE_TYPE}
)
target_include_directories(rsmeshopt INTERFACE ${rsmeshopt_SOURCE_DIR}/include)
