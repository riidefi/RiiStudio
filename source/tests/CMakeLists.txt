project(tests)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

add_executable(tests
	tests.cpp
)

set(ASSIMP_DIR, ${PROJECT_SOURCE_DIR}/../vendor/assimp)

target_link_libraries(tests PUBLIC
	core
	oishii
	plate
	plugins
	vendor
)

if (WIN32)
	target_link_libraries(tests PUBLIC
		${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3dll.lib
		${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.lib
	  opengl32.lib
	)
else()
	target_link_libraries(tests PUBLIC
		${PROJECT_SOURCE_DIR}/../vendor/assimp/libassimp.a
	  ${PROJECT_SOURCE_DIR}/../vendor/assimp/libIrrXML.a
	  ${PROJECT_SOURCE_DIR}/../vendor/assimp/libzlib.a
	  ${PROJECT_SOURCE_DIR}/../vendor/assimp/libzlibstatic.a
	)
endif()

# Decentralized installers based on initialization of static objects
#
# The clang-tidy way would be:
#
#   (In the module)
#   > volatile int MyModuleAnchorSource = 0;
#
#   (In the main binary)
#   > extern volatile int MyModuleAnchorSource;
#   > static int MyModuleAnchorDestination = MyModuleAnchorSource;
#
# I really don't like it.
#
if (MSVC)
  # clang-cl
  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    SET_TARGET_PROPERTIES(tests PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:source\\plugins\\plugins.lib")
  else()
	  SET_TARGET_PROPERTIES(tests PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:plugins")
  endif()
else()
  SET_TARGET_PROPERTIES(tests PROPERTIES LINK_FLAGS "--whole_archive")
endif()

# DLLs for windows
# if (WINDOWS)
	add_custom_command(
	  TARGET tests 
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy
		  ${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.dll
		  $<TARGET_FILE_DIR:tests>/assimp-vc141-mt.dll
	)
	add_custom_command(
	  TARGET tests 
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy
		  ${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3.dll
		  $<TARGET_FILE_DIR:tests>/glfw3.dll
	)
	add_custom_command(
	  TARGET tests 
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy_directory
		  ${PROJECT_SOURCE_DIR}/../../fonts
		  $<TARGET_FILE_DIR:tests>/fonts
	)
# endif()
