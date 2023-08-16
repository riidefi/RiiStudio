project(rsl)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(rsl STATIC
  "FsDialog.cpp"
 "Defer.hpp" "DebugBreak.hpp" "Ranges.hpp" "Stb.cpp" "SafeReader.cpp" "Launch.cpp" "Download.cpp" "Zip.cpp" "Log.cpp"
 
 "Discord.cpp"
  "WriteFile.cpp")
 
if (WIN32)
  target_link_directories(rsl PUBLIC ${PROJECT_SOURCE_DIR}/../vendor/dll)
endif()

target_link_libraries(rsl PUBLIC core riistudio_rs vendor)

if (NOT EMSCRIPTEN)
  target_link_libraries(rsl PUBLIC llvm_sighandler c-discord-rich-presence)
endif()
