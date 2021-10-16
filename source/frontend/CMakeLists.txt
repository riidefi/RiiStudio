project(frontend)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)
include_directories(${PROJECT_SOURCE_DIR}/../11zip/include)

add_executable(frontend
  "applet.cpp"
  "entry.cpp"
  "file_host.cpp"
  
  "root.cpp"
  "editor/EditorDocument.cpp"
  "editor/EditorImporter.cpp"
  "editor/EditorWindow.cpp"
  "editor/ImporterWindow.cpp"
  "editor/StudioWindow.cpp"
  "editor/views/HistoryList.cpp"
  "editor/views/Outliner.cpp"
  "editor/views/PropertyEditor.cpp"
  "editor/views/ViewportRenderer.cpp"
  "properties/gc/ResizeAction.cpp"
  "properties/gc/ShaderView.cpp"
  "properties/gc/TevSolver.cpp"
  "properties/gc/TexImageView.cpp"
  "properties/gc/Bone/BoneDisplaySurface.cpp"
  "properties/gc/Bone/BoneTransformSurface.cpp"
  "properties/gc/Material/ColorSurface.cpp"
  "properties/gc/Material/DisplaySurface.cpp"
  "properties/gc/Material/FogSurface.cpp"
  "properties/gc/Material/LightingSurface.cpp"
  "properties/gc/Material/PixelSurface.cpp"
  "properties/gc/Material/SamplerSurface.cpp"
  "properties/gc/Material/StageSurface.cpp"
  "properties/gc/Material/SwapTableSurface.cpp"
  "properties/gc/Polygon/PolyDataSurface.cpp"
  "properties/gc/Polygon/PolyDescriptorSurface.cpp"
  "renderer/Camera.cpp"
  "renderer/CameraController.cpp"
  "renderer/Renderer.cpp"
  "updater/updater.cpp"
  "widgets/IconDatabase.cpp"
  "widgets/IconManager.cpp"
  "widgets/Image.cpp"

  ${PROJECT_SOURCE_DIR}/rc/RiiStudio.rc
 "renderer/InputState.hpp" "renderer/InputState.cpp" "properties/gc/Material/Indirect.cpp" "Localization.cpp" "Localization.hpp" "editor/views/OutlinerWidget.hpp" "editor/views/OutlinerWidget.cpp" "LeakDebug.hpp" "Fonts.hpp" "level_editor/LevelEditor.hpp" "level_editor/LevelEditor.cpp" "level_editor/ObjUtil.hpp" "level_editor/ObjUtil.cpp" "level_editor/ColorUtil.hpp" "level_editor/KclUtil.hpp" "level_editor/KclUtil.cpp" "level_editor/Archive.hpp" "level_editor/Archive.cpp" "level_editor/IO.hpp" "level_editor/IO.cpp" "level_editor/Transform.cpp" "level_editor/Transform.hpp")

set(ASSIMP_DIR, ${PROJECT_SOURCE_DIR}/../vendor/assimp)

target_link_libraries(frontend PUBLIC
	core
	oishii
	plate
  librii
	plugins
	vendor
)

if (WIN32)
  target_link_libraries(frontend PUBLIC
    elzip
  )
endif()

if (WIN32)
  set(LINK_LIBS
		${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3dll.lib
		${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.lib
		opengl32.lib
    ${PROJECT_SOURCE_DIR}/../vendor/libcurl.lib
	)
  if (ASAN)
    set(LINK_LIBS ${LINK_LIBS} "C:\\Program Files\\LLVM\\lib\\clang\\10.0.0\\lib\\windows\\clang_rt.asan-x86_64.lib")
  endif()
  
	target_link_libraries(frontend PUBLIC ${LINK_LIBS})
elseif (NOT UNIX)
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
    SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "-defaultlib:libcmt /WHOLEARCHIVE:source\\plugins\\plugins.lib")
  else()
	  SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:plugins")
  endif()
else()
  if (APPLE)
    execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
    message(STATUS "Architecture: ${ARCHITECTURE}")
    if (${ARCHITECTURE} STREQUAL "arm64")
  	  SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "-framework CoreFoundation -ldl /opt/homebrew/Cellar/assimp/5.0.1/lib/libassimp.dylib /opt/homebrew/Cellar/glfw/3.3.4/lib/libglfw.dylib")
    else()
  	  SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "-framework CoreFoundation -ldl /usr/local/Cellar/assimp/5.0.1/lib/libassimp.dylib /usr/local/Cellar/glfw/3.3.4/lib/libglfw.dylib")
    endif()
  elseif (UNIX)
    # --start-group, --end-group allows circular dependencies among object files
    SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "-Wl,--start-group -ldl -lassimp -lglfw -lstdc++ -lm")
    # This is a hack to append a final link arg
    target_link_libraries(frontend PUBLIC "-Wl,--end-group")
  else()
    SET_TARGET_PROPERTIES(frontend PROPERTIES LINK_FLAGS "--whole_archive")
  endif()
  # TODO: Not functional (yet)
	#add_link_options("-s USE_SDL=2 -s MAX_WEBGL_VERSION=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 --bind")
	#add_link_options("-s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 --no-heap-copy --preload-file ./fonts@/fonts")
  #add_link_options("-o out.html --shell-file shell_minimal.html")
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
    COMMAND ${CMAKE_COMMAND} -E copy
      ${PROJECT_SOURCE_DIR}/../vendor/msdia140.dll
      $<TARGET_FILE_DIR:frontend>/msdia140.dll
  )
  add_custom_command(
	  TARGET frontend 
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E copy
		  ${PROJECT_SOURCE_DIR}/../vendor/libcurl.dll
		  $<TARGET_FILE_DIR:frontend>/libcurl.dll
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

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "RiiStudio")
