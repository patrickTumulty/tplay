cmake_minimum_required(VERSION 3.20)

project(tplay LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

add_executable(tplay src/main.cpp)
target_compile_features(tplay PRIVATE cxx_std_17)
target_link_libraries(tplay PRIVATE PkgConfig::GSTREAMER)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tplay PRIVATE -Wall -Wextra -Wpedantic)
endif()
