file(GLOB SOURCES "src/*.h" "src/*.cpp" "src/FreeImage/*.cpp" "src/FreeImageToolkit/*.cpp" "src/Metadata/*.cpp" "src/LibJPEG/*.c" "src/LibPNG/*.c" "src/LibWebP/src/*/*.h" "src/LibWebP/src/*/*.c")
set(TARGET FreeImage)
include(${CMAKE_SOURCE_DIR}/cmake/lib.cmake)

set_target_properties(${TARGET} PROPERTIES COMPILE_DEFINITIONS "FREEIMAGE_EXPORTS")
target_include_directories(${TARGET} PRIVATE src src/LibWebP ${CMAKE_SOURCE_DIR}/libs/zlib/src)
target_link_libraries(${TARGET} Zlib)