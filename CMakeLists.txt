cmake_minimum_required(VERSION 3.21)
project(tessera VERSION 0.1.0 LANGUAGES CXX)

# Tests build only when tessera is the top-level project, never inside a project that adds it.
option(TESSERA_BUILD_TESTS "Build tessera's tests" ${PROJECT_IS_TOP_LEVEL})
option(TESSERA_WARNINGS_AS_ERRORS "Fail the build on compiler warnings" ON)

function(tessera_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(TESSERA_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow)
        # GCC's -Wshadow and MSVC's /W4 flag a local that hides a namespace-scope variable; Clang needs
        # -Wshadow-all for that, so every compiler reports the same shadowing.
        target_compile_options(${target} PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wshadow-all>)
        if(TESSERA_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# Pure procedural raster core: no dependencies beyond the standard library.
add_library(tessera
    src/Adjust.cpp
    src/Animation.cpp
    src/Canvas.cpp
    src/Color.cpp
    src/Compose.cpp
    src/Dither.cpp
    src/Draw.cpp
    src/Filter.cpp
    src/Gradient.cpp
    src/Image.cpp
    src/Isometric.cpp
    src/NineSlice.cpp
    src/Noise.cpp
    src/Palette.cpp
    src/Sheet.cpp
    src/Sprite.cpp
    src/Text.cpp
    src/Tilemap.cpp
    src/Transform.cpp
    src/Vector.cpp
    src/VFX.cpp)
add_library(tessera::tessera ALIAS tessera)
target_include_directories(tessera PUBLIC include)
target_compile_features(tessera PUBLIC cxx_std_20)
set_target_properties(tessera PROPERTIES CXX_EXTENSIONS OFF)
tessera_warnings(tessera)

if(TESSERA_BUILD_TESTS)
    enable_testing()
    add_executable(tessera_tests
        tests/adjust_tests.cpp
        tests/canvas_tests.cpp
        tests/compose_tests.cpp
        tests/draw_tests.cpp
        tests/filter_tests.cpp
        tests/image_tests.cpp
        tests/isometric_tests.cpp
        tests/main.cpp
        tests/palette_tests.cpp
        tests/procedural_tests.cpp
        tests/sprite_tests.cpp
        tests/text_tests.cpp
        tests/transform_tests.cpp
        tests/vector_tests.cpp)
    target_link_libraries(tessera_tests PRIVATE tessera)
    target_include_directories(tessera_tests SYSTEM PRIVATE third_party/doctest)
    tessera_warnings(tessera_tests)
    add_test(NAME tessera_tests COMMAND tessera_tests)
endif()
