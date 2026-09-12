cmake_minimum_required(VERSION 3.20)

project(tplay LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wno-unused-variable
    -Wno-unused-parameter
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${GSTREAMER_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${GSTREAMER_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GSTREAMER_LIBRARIES}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
