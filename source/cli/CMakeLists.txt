project(cli)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

add_executable(cli
  "Cli.cpp"
  "Rszst.cpp"
 )


target_link_libraries(cli PUBLIC
  core
  librii
  oishii
  rsl
  plate
  plugins
  vendor
  rsmeshopt
  rszst_arg_parser
)
# ???
add_dependencies(cli plugins)

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
   
  target_link_libraries(cli PUBLIC ${LINK_LIBS})
elseif (APPLE)
  execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
  message(STATUS "Architecture: ${ARCHITECTURE}")
 
  if (${ARCHITECTURE} STREQUAL "arm64")
    set(HOMEBREW_CELLAR "/opt/homebrew/Cellar")
  else()
    set(HOMEBREW_CELLAR "/usr/local/Cellar")
  endif()
 
  set(GLFW_VERSION "3.4")
  set(FREETYPE_VERSION "2.13.2")
 
  SET_TARGET_PROPERTIES(cli PROPERTIES LINK_FLAGS "-framework CoreFoundation -framework Security -ldl ${HOMEBREW_CELLAR}/glfw/${GLFW_VERSION}/lib/libglfw.dylib ${HOMEBREW_CELLAR}/freetype/${FREETYPE_VERSION}/lib/libfreetype.dylib")
elseif (UNIX AND NOT EMSCRIPTEN)
  find_library(GLFW3_LIBRARY NAMES libglfw.so PATHS /usr/lib)
  find_library(FREETYPE_LIBRARY NAMES libfreetype.so PATHS /usr/lib)
  find_library(BZIP2_LIBRARY NAMES libbz2.so PATHS /usr/lib)
  find_library(SSL_LIBRARY NAMES libssl.so PATHS /usr/lib)
  find_library(CRYPTO_LIBRARY NAMES libcrypto.so PATHS /usr/lib)
  target_link_libraries(cli PUBLIC
    ${GLFW3_LIBRARY}
    ${FREETYPE_LIBRARY}
    ${BZIP2_LIBRARY}
    ${SSL_LIBRARY}
    ${CRYPTO_LIBRARY}
  )
endif()
 
# DLLs for windows
if (WIN32)
  add_custom_command(
    TARGET cli 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      ${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.dll
      $<TARGET_FILE_DIR:cli>/assimp-vc141-mt.dll
  )
  add_custom_command(
    TARGET cli 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      ${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3.dll
      $<TARGET_FILE_DIR:cli>/glfw3.dll
  )
  add_custom_command(
    TARGET cli 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/../vendor/dll
      $<TARGET_FILE_DIR:cli>
  )
endif()
add_custom_command(
  TARGET cli 
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/../../fonts
    $<TARGET_FILE_DIR:cli>/fonts
)
add_dependencies(cli rsl)

# Disabled: No DLLs exist
if (WIN32) # AND CMAKE_BUILD_TYPE STREQUAL "Debug"
  add_custom_command(TARGET cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:cli> $<TARGET_FILE_DIR:cli> 
    COMMAND_EXPAND_LISTS
  )
endif()
 
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "rszst")
