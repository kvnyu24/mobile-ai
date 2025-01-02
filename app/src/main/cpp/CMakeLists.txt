cmake_minimum_required(VERSION 3.18.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ANDROID_NDK "/opt/homebrew/share/android-ndk")
set(ANDROID_PLATFORM 21)
set(ANDROID_ABI arm64-v8a)
set(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")

project("mobileai")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add dependencies using FetchContent
include(FetchContent)

# Add nlohmann-json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Add jsoncpp
FetchContent_Declare(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.5
)
FetchContent_MakeAvailable(jsoncpp)

# Add source files
file(GLOB_RECURSE SOURCES
    "core/*.cpp" "core/*.h"
    "conversion/*.cpp" "conversion/*.h"
    "hardware/*.cpp" "hardware/*.h"
    "inference/*.cpp" "inference/*.h"
    "monitoring/*.cpp" "monitoring/*.h"
    "benchmark/*.cpp" "benchmark/*.h"
)

# Create shared library
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

# Add include directories
target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${ANDROID_NDK}/sources/android/log/include
    ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include
    ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/c++/v1
    ${ANDROID_NDK}/sources/tensorflow/include
)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    jsoncpp_lib
    android
    log
    ${ANDROID_NDK}/sources/tensorflow/libs/${ANDROID_ABI}/libtensorflowlite.so
    nlohmann_json::nlohmann_json
)