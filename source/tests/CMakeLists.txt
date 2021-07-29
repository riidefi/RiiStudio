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
  librii
	oishii
	plate
	plugins
	vendor
)

if (WIN32)
  set(LINK_LIBS
		${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3dll.lib
		${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.lib
		opengl32.lib
	)
  if (ASAN)
    set(LINK_LIBS ${LINK_LIBS} "C:\\Program Files\\LLVM\\lib\\clang\\10.0.0\\lib\\windows\\clang_rt.asan-x86_64.lib")
  endif()
  
	target_link_libraries(tests PUBLIC ${LINK_LIBS})
elseif (APPLE)
  	SET_TARGET_PROPERTIES(tests PROPERTIES LINK_FLAGS "-framework CoreFoundation -ldl /opt/homebrew/Cellar/assimp/5.0.1/lib/libassimp.dylib /opt/homebrew/Cellar/glfw/3.3.4/lib/libglfw.dylib")
		#target_link_libraries(tests PUBLIC plugins "-Wl,-force_load source\\plugins\\libplugins.a")
elseif (UNIX)
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
	if (APPLE)
	else()
  	SET_TARGET_PROPERTIES(tests PROPERTIES LINK_FLAGS "--whole_archive")
  endif()
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

if (WINDOWS)
  add_custom_command(
	  TARGET tests 
	  POST_BUILD
	  COMMAND python.exe ${PROJECT_SOURCE_DIR}/../../tests/tests.py
		  $<TARGET_FILE_DIR:tests>/tests.exe
		  ${PROJECT_SOURCE_DIR}/../../tests/samples
      ${PROJECT_SOURCE_DIR}/../../tests/out
	)
else()
  add_custom_command(
	  TARGET tests 
	  POST_BUILD
	  COMMAND python3 ${PROJECT_SOURCE_DIR}/../../tests/tests.py
		  $<TARGET_FILE_DIR:tests>/tests
		  ${PROJECT_SOURCE_DIR}/../../tests/samples
      ${PROJECT_SOURCE_DIR}/../../tests/out
	)
endif()
# endif()
