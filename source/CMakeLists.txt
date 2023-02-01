include(${CMAKE_CURRENT_SOURCE_DIR}/Configurations.cmake)

add_subdirectory(core)
add_subdirectory(LibBadUIFramework)
add_subdirectory(updater)
add_subdirectory(plugins)

# My libraries
add_subdirectory(oishii)
add_subdirectory(plate)
add_subdirectory(rsl)
add_subdirectory(librii)

# Third-party libraries
add_subdirectory(vendor)
add_subdirectory(minizip)

add_subdirectory(tests)
add_subdirectory(frontend)
