project(frontend)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)
include_directories(${PROJECT_SOURCE_DIR}/../11zip/include)

add_library(lib_frontend
  "applet.cpp"
  "Frontend.cpp"
  "file_host.cpp"
  
  "root.cpp"
  "widgets/utilities.hpp" "widgets/utilities.cpp"
  
  "legacy_editor/EditorWindow.cpp"
  
  "legacy_editor/StudioWindow.cpp"
  
  "legacy_editor/views/Outliner.cpp"
  "legacy_editor/views/PropertyEditor.cpp"
  
  "widgets/OutlinerWidget.cpp"
  "properties/gc/ResizeAction.cpp"
  "properties/gc/ShaderView.cpp"
  "properties/gc/TexImageView.cpp"
  "properties/gc/Bone/BoneDisplaySurface.cpp"
  "properties/gc/Bone/BoneTransformSurface.cpp"
  "properties/gc/Material/ColorSurface.cpp"
  "properties/gc/Material/DisplaySurface.cpp"
  
  "properties/gc/Material/LightingSurface.cpp"
  "properties/gc/Material/PixelSurface.cpp"
  "properties/gc/Material/SamplerSurface.cpp"
  "properties/gc/Material/StageSurface.cpp"
  "properties/gc/Material/SwapTableSurface.cpp"
  "properties/gc/Polygon/PolyDataSurface.cpp"
  "properties/gc/Polygon/PolyDescriptorSurface.cpp"
 "properties/gc/Material/CratePreset.cpp"
 "properties/gc/Material/Indirect.cpp"
  
  "properties/g3d/G3dSrtView.cpp"
  "properties/g3d/G3dSrtCurveEditor.cpp"
  "properties/g3d/G3dVertexColorView.cpp"
	"properties/j3d/MaterialView.cpp"
  "properties/g3d/G3dUi.cpp"
  "renderer/Camera.cpp"
  "renderer/CameraController.cpp"
  "renderer/Renderer.cpp"
  "widgets/IconDatabase.cpp"
  "widgets/IconManager.cpp"
  "widgets/Image.cpp"

 "renderer/InputState.hpp" "renderer/InputState.cpp"
 "Localization.cpp" "Localization.hpp"
 "Fonts.hpp"

 "level_editor/LevelEditor.hpp" "level_editor/LevelEditor.cpp"
 "level_editor/ObjUtil.hpp" "level_editor/ObjUtil.cpp"
 "level_editor/KclUtil.hpp" "level_editor/KclUtil.cpp"
 "level_editor/Archive.hpp" "level_editor/Archive.cpp"
 "level_editor/IO.hpp" "level_editor/IO.cpp"
 "level_editor/Transform.cpp" "level_editor/Transform.hpp"
 "level_editor/TriangleRenderer.cpp" "level_editor/TriangleRenderer.hpp"
 "level_editor/ViewCube.hpp"
 "level_editor/ViewCube.cpp"

 "widgets/AutoHistory.hpp"
 "widgets/DeltaTime.hpp"
 "widgets/DeltaTime.cpp"
 "widgets/CppSupport.hpp"
 "widgets/CppSupport.cpp"
 "widgets/Lib3dImage.hpp"
 "widgets/Lib3dImage.cpp"
 "widgets/Manipulator.hpp"
 "widgets/Manipulator.cpp"

 "UpdaterView.cpp"
 "Fonts.cpp"
 "EditorFactory.cpp"

 "editors/RarcEditor.cpp"
 "editors/AssimpImporter.cpp"
 "editors/BblmEditor.cpp"
 "editors/BdofEditor.cpp"
 "editors/BfgEditor.cpp"
 "editors/BlightEditor.cpp"
 "editors/BlmapEditor.cpp"
 "editors/BtkEditor.cpp"
 
 "properties/gc/Material/AdvTexConv.cpp"
 "properties/gc/Material/Merge.cpp"
 "editors/BtkEditor.cpp"
 "legacy_editor/views/BmdBrresOutliner.cpp" "renderer/MouseHider.cpp" "editors/MkwDebug.cpp" "PresetHelper.cpp")

add_executable(frontend 
 "entry.cpp"
  ${PROJECT_SOURCE_DIR}/rc/RiiStudio.rc)

target_link_libraries(lib_frontend PUBLIC
	core
	oishii
	plate
  librii
  rsl
	plugins
	vendor
  LibBadUIFramework
  updater
  rsmeshopt
)
target_link_libraries(frontend PUBLIC
  lib_frontend
)

find_package(OpenGL REQUIRED)

if (WIN32)
  set(LINK_LIBS
    ${LINK_LIBS}
		${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3dll.lib
		OpenGL::GL
    ntdll Crypt32 Secur32 Ncrypt # Rust needs this now?
    ${PROJECT_SOURCE_DIR}/../vendor/freetype.lib
	)
  if (ASAN)
    set(LINK_LIBS ${LINK_LIBS} "C:\\Program Files\\LLVM\\lib\\clang\\10.0.0\\lib\\windows\\clang_rt.asan-x86_64.lib")
  endif()
  
	target_link_libraries(frontend PUBLIC ${LINK_LIBS})
elseif (UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
  # TODO: Duplication: extract to FindX.cmake
  find_library(BZIP2_LIBRARY NAMES libbz2.so PATHS /usr/lib)
  find_library(SSL_LIBRARY NAMES libssl.so PATHS /usr/lib)
  find_library(CRYPTO_LIBRARY NAMES libcrypto.so PATHS /usr/lib)
  target_link_libraries(frontend PUBLIC ${BZIP2_LIBRARY} ${SSL_LIBRARY} ${CRYPTO_LIBRARY})
endif()

if (NOT MSVC)
  if (APPLE)
    execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
    message(STATUS "Architecture: ${ARCHITECTURE}")

    if (${ARCHITECTURE} STREQUAL "arm64")
      set(HOMEBREW_CELLAR "/opt/homebrew/Cellar")
    else()
      set(HOMEBREW_CELLAR "/usr/local/Cellar")
    endif()

    set(GLFW_VERSION "3.3.8")
    set(FREETYPE_VERSION "2.13.2")

    SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "-framework CoreFoundation -framework Security -ldl ${HOMEBREW_CELLAR}/glfw/${GLFW_VERSION}/lib/libglfw.dylib ${HOMEBREW_CELLAR}/freetype/${FREETYPE_VERSION}/lib/libfreetype.dylib")
  elseif (UNIX)
    # --start-group, --end-group allows circular dependencies among object files
    if (EMSCRIPTEN)
      SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "-Wl,--start-group -ldl -lglfw -lstdc++ -lm")
    else()
      SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "-Wl,--start-group -ldl -lglfw -lfreetype -lstdc++ -lm")
    endif()
    # This is a hack to append a final link arg
    target_link_libraries(frontend PUBLIC "-Wl,--end-group")
  endif()
  # TODO: Not functional (yet)
	#add_link_options("-s USE_SDL=2 -s MAX_WEBGL_VERSION=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 --bind")
	#add_link_options("-s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 --no-heap-copy --preload-file ./fonts@/fonts")
  #add_link_options("-o out.html --shell-file shell_minimal.html")
endif()

if (EMSCRIPTEN)
  # Debugging  -g4 --source-map-base ../ -O0
  set_target_properties(frontend PROPERTIES LINK_FLAGS "-s USE_FREETYPE=1 -s MAX_WEBGL_VERSION=2 --bind -sASSERTIONS --preload-file ../fonts@fonts -s ALLOW_MEMORY_GROWTH=1  --preload-file ../lang@lang --preload-file ../samp@samp")
endif()

# DLLs for windows
if (WIN32)
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
		  ${PROJECT_SOURCE_DIR}/../vendor/assimp/zlib1.dll
		  $<TARGET_FILE_DIR:frontend>/zlib1.dll
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
		  ${PROJECT_SOURCE_DIR}/../vendor/dll
		  $<TARGET_FILE_DIR:frontend>
  )
endif()

add_custom_command(
	TARGET frontend 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/../../fonts
		$<TARGET_FILE_DIR:frontend>/fonts
)

add_custom_command(
	TARGET frontend 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/../../lang
		$<TARGET_FILE_DIR:frontend>/lang
)

add_custom_command(
TARGET frontend 
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy
	${PROJECT_SOURCE_DIR}/rc/icon.png
	$<TARGET_FILE_DIR:frontend>/icon.png
)

# Copy Blender plugin
add_custom_command(
TARGET frontend 
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy
	${PROJECT_SOURCE_DIR}/../blender/riistudio_blender.py
	$<TARGET_FILE_DIR:frontend>/riistudio_blender.py
)

if (NOT EMSCRIPTEN)
  # Ensure tests is built first
  add_dependencies(frontend tests cli)

  add_custom_command(
  TARGET frontend 
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
	  $<TARGET_FILE:tests>
	  $<TARGET_FILE_DIR:frontend>/
  )

  # Windows: Copy .pdb with debug info
  if (WIN32)
    add_custom_command(
    TARGET frontend 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
	    $<TARGET_FILE_DIR:tests>/tests.pdb
	    $<TARGET_FILE_DIR:frontend>/tests.pdb
    )
  endif()

  add_custom_command(
    TARGET frontend 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
	    $<TARGET_FILE:cli>
	    $<TARGET_FILE_DIR:frontend>/
  )

  # Windows: Copy .pdb with debug info
  if (WIN32)
    add_custom_command(
    TARGET frontend 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
	    $<TARGET_FILE_DIR:cli>/rszst.pdb
	    $<TARGET_FILE_DIR:frontend>/rszst.pdb
    )
  endif()
endif()

# Disabled: No DLLs exist
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_custom_command(TARGET frontend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:frontend> $<TARGET_FILE_DIR:frontend> 
    COMMAND_EXPAND_LISTS
  )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "RiiStudio")

# CMake w/ ARM MacOS GCC does not work with PCH. Passes "-Xarch_arm" which is not valid on GCC.
if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_precompile_headers(frontend
  PUBLIC
    ../core/common.h
  )
endif()

add_dependencies(frontend rsl)
