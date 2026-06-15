cmake_minimum_required(VERSION 3.16)
project(tessera CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pure procedural raster core: no dependencies beyond the standard library.
file(GLOB TESSERA_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(tessera ${TESSERA_SOURCES})
add_library(tessera::tessera ALIAS tessera)

target_include_directories(tessera PUBLIC include)
target_compile_features(tessera PUBLIC cxx_std_20)
target_compile_options(tessera PRIVATE -Wall -Wextra)
