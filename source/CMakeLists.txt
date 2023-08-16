include(${CMAKE_CURRENT_SOURCE_DIR}/Configurations.cmake)

add_subdirectory(core)

# My libraries
add_subdirectory(plate)
add_subdirectory(oishii)
add_subdirectory(rsl)
add_subdirectory(librii)
add_subdirectory(LibBadUIFramework)

# Third-party libraries
add_subdirectory(vendor)

add_subdirectory(updater)
add_subdirectory(plugins)
if (NOT EMSCRIPTEN)
  add_subdirectory(tests)
  add_subdirectory(cli)
endif()
add_subdirectory(frontend)

add_subdirectory(rsmeshopt)
