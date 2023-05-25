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
add_subdirectory(tests)
add_subdirectory(frontend)
add_subdirectory(cli)

add_subdirectory(rsmeshopt)
