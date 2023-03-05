project(cli)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

add_executable(cli
  "Cli.cpp"
  "Rszst.cpp"
 )

 set(ASSIMP_DIR, ${PROJECT_SOURCE_DIR}/../vendor/assimp)

 target_link_libraries(cli PUBLIC
   core
   librii
   oishii
   rsl
   plate
   plugins
   vendor
 )
 # ???
 add_dependencies(cli plugins)
 
 if (WIN32)
   set(LINK_LIBS
     ${PROJECT_SOURCE_DIR}/../plate/vendor/glfw/lib-vc2017/glfw3dll.lib
     ${PROJECT_SOURCE_DIR}/../vendor/assimp/assimp-vc141-mt.lib
     opengl32.lib
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
 
     set(ASSIMP_VERSION "5.2.5")
     set(GLFW_VERSION "3.3.8")
   set(FREETYPE_VERSION "2.13.0")
 
     SET_TARGET_PROPERTIES(cli PROPERTIES LINK_FLAGS "-framework CoreFoundation -ldl ${HOMEBREW_CELLAR}/assimp/${ASSIMP_VERSION}/lib/libassimp.dylib ${HOMEBREW_CELLAR}/glfw/${GLFW_VERSION}/lib/libglfw.dylib ${HOMEBREW_CELLAR}/freetype/${FREETYPE_VERSION}/lib/libfreetype.dylib")
 elseif (NOT UNIX)
   target_link_libraries(cli PUBLIC
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
     SET_TARGET_PROPERTIES(cli PROPERTIES LINK_FLAGS "-defaultlib:libcmt /WHOLEARCHIVE:source\\plugins\\plugins.lib")
   else()
     SET_TARGET_PROPERTIES(cli PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:plugins")
   endif()
 else()
   if (APPLE)
   elseif (UNIX)
     # --start-group, --end-group allows circular dependencies among object files
     SET_TARGET_PROPERTIES(cli PROPERTIES LINK_FLAGS "-Wl,--start-group -ldl -lassimp -lglfw -lfreetype -lstdc++ -lm -lpthread")
     # This is a hack to append a final link arg
     target_link_libraries(cli PUBLIC "-Wl,--end-group")
   else()
     # SET_TARGET_PROPERTIES(cli PROPERTIES LINK_FLAGS "--whole_archive")
   endif()
 endif()
 
 # DLLs for windows
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
       ${PROJECT_SOURCE_DIR}/../../fonts
       $<TARGET_FILE_DIR:cli>/fonts
   )
   add_custom_command(
     TARGET cli 
     POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_directory
       ${PROJECT_SOURCE_DIR}/../vendor/dll
       $<TARGET_FILE_DIR:cli>
   )
 

   set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "rszst")