cmake_minimum_required(VERSION 3.18.1)

# Set Android NDK and toolchain
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ANDROID_PLATFORM 21)
set(ANDROID_ABI arm64-v8a)

# Set Android STL
set(ANDROID_STL c++_shared)
set(ANDROID_CPP_FEATURES exceptions rtti)

project("mobileai")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable TensorFlow Lite to get the build working
add_definitions(-DDISABLE_TENSORFLOW_LITE)

# Include directories - add third_party directory first to ensure our stub headers are used
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tensorflow)

# Add dependencies using FetchContent
include(FetchContent)

# Add nlohmann-json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Add jsoncpp with tests disabled
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Disable jsoncpp tests" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Disable jsoncpp post-build tests" FORCE)
FetchContent_Declare(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.5
)
FetchContent_MakeAvailable(jsoncpp)

# Define TensorFlow Lite directory
set(TENSORFLOW_LITE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tensorflow/tensorflow/lite")

# Add TensorFlow Lite if directory exists
if(EXISTS "${TENSORFLOW_LITE_DIR}")
    message(STATUS "TensorFlow Lite directory found at ${TENSORFLOW_LITE_DIR}")
else()
    message(WARNING "TensorFlow Lite directory not found at ${TENSORFLOW_LITE_DIR}. Creating directory structure...")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tensorflow/tensorflow/lite")
endif()

# Add source files - exclude problematic folders
file(GLOB_RECURSE SOURCES
    "core/*.cpp" "core/*.h"
    "conversion/*.cpp" "conversion/*.h"
    # Temporarily disable hardware accelerator code
    # "hardware/*.cpp" "hardware/*.h"
    # Temporarily disable TensorFlow Lite inference code and benchmark code
    # "inference/*.cpp" "inference/*.h"
    "monitoring/*.cpp" "monitoring/*.h"
    # "benchmark/*.cpp" "benchmark/*.h"
    "optimization/*.cpp" "optimization/*.h"
)

# Create shared library
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

# Add include directories
target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${ANDROID_NDK}/sources/android/log/include
    ${json_SOURCE_DIR}/include
    ${jsoncpp_SOURCE_DIR}/include
    third_party/jsoncpp/include
    third_party/onnxruntime/include
)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    jsoncpp_lib
    android
    log
    nlohmann_json::nlohmann_json
    # Removing references to libraries that caused conflicts
    # Will use JNI to access these libraries from Android Java
    # tensorflow-lite
    # torch
    # onnxruntime
    # ${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/lib/${ANDROID_ABI}/libonnxruntime.so
)