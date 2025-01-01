cmake_minimum_required(VERSION 3.18.1)

project("mobileai")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${ANDROID_NDK}/sources/android/log/include
)

# Add source files
file(GLOB_RECURSE SOURCES
    "core/*.cpp"
    "conversion/*.cpp"
    "hardware/*.cpp"
    "inference/*.cpp"
    "monitoring/*.cpp"
    "benchmark/*.cpp"
)

# Create shared library
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    ${JSONCPP_LIBRARIES}
    android
    log
)