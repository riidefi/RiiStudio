project(frontend)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

file(GLOB_RECURSE SRC_FILES
	${PROJECT_SOURCE_DIR}/*.cpp
)
file(GLOB_RECURSE HDR_FILES
	${PROJECT_SOURCE_DIR}/*.hpp
	${PROJECT_SOURCE_DIR}/*.h
)

add_executable(frontend
	${SRC_FILES}
	${HDR_FILES}
)

set(ASSIMP_DIR, ${PROJECT_SOURCE_DIR}/../vendor/assimp)

target_link_libraries(frontend PUBLIC
	core
	oishii
	plate
	plugins
	vendor
)

if (WIN32)
	target_link_libraries(frontend PUBLIC
		${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3dll.lib
		${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.lib
	  opengl32.lib
	)
else()
	target_link_libraries(frontend PUBLIC
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
    SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:source\\plugins\\plugins.lib")
  else()
	  SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:plugins")
  endif()
else()
  SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "--whole_archive")
  # TODO: Not functional (yet)
	#add_link_options("-s USE_SDL=2 -s MAX_WEBGL_VERSION=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 --bind")
	#add_link_options("-s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 --no-heap-copy --preload-file ./fonts@/fonts")
  #add_link_options("-o out.html --shell-file shell_minimal.html")
endif()

# DLLs for windows
# if (WINDOWS)
	add_custom_command(
	  TARGET frontend 
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy
		  ${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.dll
		  $<TARGET_FILE_DIR:frontend>/assimp-vc141-mt.dll
	)
	add_custom_command(
	  TARGET frontend 
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy
		  ${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3.dll
		  $<TARGET_FILE_DIR:frontend>/glfw3.dll
	)
	add_custom_command(
	  TARGET frontend 
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy_directory
		  ${PROJECT_SOURCE_DIR}/../../fonts
		  $<TARGET_FILE_DIR:frontend>/fonts
	)
# endif()
