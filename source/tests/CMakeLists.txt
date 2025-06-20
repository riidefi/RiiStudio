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
	rsl
	plate
	plugins
	vendor
  rsmeshopt
)
# Since we rely on rszst.exe
add_dependencies(tests cli)
# ???
add_dependencies(tests plugins)

add_dependencies(tests rsl)

if (WIN32)
  set(LINK_LIBS
		${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3dll.lib
		opengl32.lib
    ntdll Crypt32 Secur32 Ncrypt # Rust needs this now?
		${PROJECT_SOURCE_DIR}/../vendor/freetype.lib
	)
  if (ASAN)
    set(LINK_LIBS ${LINK_LIBS} "C:\\Program Files\\LLVM\\lib\\clang\\10.0.0\\lib\\windows\\clang_rt.asan-x86_64.lib")
  endif()
  
	target_link_libraries(tests PUBLIC ${LINK_LIBS})
elseif (APPLE)
    execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
    message(STATUS "Architecture: ${ARCHITECTURE}")

    if (${ARCHITECTURE} STREQUAL "arm64")
      set(HOMEBREW_CELLAR "/opt/homebrew/Cellar")
    else()
      set(HOMEBREW_CELLAR "/usr/local/Cellar")
    endif()

    set(ASSIMP_VERSION "6.0.1")
    set(GLFW_VERSION "3.4")
	set(FREETYPE_VERSION "2.13.3")

    SET_TARGET_PROPERTIES(tests PROPERTIES LINK_FLAGS "-framework CoreFoundation -framework Security -framework SystemConfiguration -ldl ${HOMEBREW_CELLAR}/glfw/${GLFW_VERSION}/lib/libglfw.dylib ${HOMEBREW_CELLAR}/freetype/${FREETYPE_VERSION}/lib/libfreetype.dylib")
elseif(UNIX AND NOT EMSCRIPTEN)
	# TODO: code duplication, extract FindBzip2.cmake to top level CMakeLists
	find_library(BZIP2_LIBRARY NAMES libbz2.so PATHS /usr/lib)
	find_library(SSL_LIBRARY NAMES libssl.so PATHS /usr/lib)
	find_library(CRYPTO_LIBRARY NAMES libcrypto.so PATHS /usr/lib)
	target_link_libraries(tests PUBLIC ${BZIP2_LIBRARY} ${SSL_LIBRARY} ${CRYPTO_LIBRARY})
endif()

if (UNIX AND NOT APPLE)
  # --start-group, --end-group allows circular dependencies among object files
  SET_TARGET_PROPERTIES(tests PROPERTIES LINK_FLAGS "-Wl,--start-group -ldl -lglfw -lfreetype -lstdc++ -lm -lpthread")
  # This is a hack to append a final link arg
  target_link_libraries(tests PUBLIC "-Wl,--end-group")
endif()

# DLLs for windows
if (WIN32)
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
			${PROJECT_SOURCE_DIR}/../vendor/dll
			$<TARGET_FILE_DIR:tests>
	)
  # Disabled: No DLLs exist
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	  add_custom_command(TARGET tests POST_BUILD
	    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:tests> $<TARGET_FILE_DIR:tests> 
	    COMMAND_EXPAND_LISTS
	  )
  endif()
endif()

add_custom_command(
	TARGET tests 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/../../fonts
		$<TARGET_FILE_DIR:tests>/fonts
)

if (WIN32)
  add_custom_command(
	  TARGET tests
	  POST_BUILD
	  COMMAND python.exe ${PROJECT_SOURCE_DIR}/../../tests/tests.py
      $<TARGET_FILE_DIR:tests>/tests
      $<TARGET_FILE_DIR:cli>/rszst
      ${PROJECT_SOURCE_DIR}/../../tests/samples
      ${PROJECT_SOURCE_DIR}/../../tests/out
	)
else()
  add_custom_command(
	  TARGET tests 
	  POST_BUILD
	  COMMAND python3 ${PROJECT_SOURCE_DIR}/../../tests/tests.py
		  $<TARGET_FILE_DIR:tests>/tests
      $<TARGET_FILE_DIR:cli>/rszst
		  ${PROJECT_SOURCE_DIR}/../../tests/samples
      ${PROJECT_SOURCE_DIR}/../../tests/out
	)
endif()
# endif()
