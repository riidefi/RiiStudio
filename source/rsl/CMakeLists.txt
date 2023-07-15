project(rsl)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(rsl STATIC
  "FsDialog.cpp"
 "Defer.hpp" "DebugBreak.hpp" "Ranges.hpp" "Stb.cpp" "SafeReader.cpp" "Launch.cpp" "Download.cpp" "Zip.cpp" "Log.cpp"
 
 "Discord.cpp"
 )
 
if (WIN32)
  target_link_directories(rsl PUBLIC ${PROJECT_SOURCE_DIR}/../vendor/dll)
endif()

target_link_libraries(rsl PUBLIC core range-v3 riistudio_rs vendor llvm_sighandler c-discord-rich-presence)
