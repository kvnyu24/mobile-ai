cmake_minimum_required(VERSION 3.18.1)

# Set Android NDK and toolchain
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ANDROID_NDK "/opt/homebrew/share/android-ndk")
set(ANDROID_PLATFORM 21)
set(ANDROID_ABI arm64-v8a)
set(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")

# Set Android STL
set(ANDROID_STL c++_shared)
set(ANDROID_CPP_FEATURES exceptions rtti)

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

# Add TensorFlow Lite
FetchContent_Declare(
    tensorflow_lite
    GIT_REPOSITORY https://github.com/tensorflow/tensorflow.git
    GIT_TAG v2.15.0
)
FetchContent_GetProperties(tensorflow_lite)
if(NOT tensorflow_lite_POPULATED)
    FetchContent_Populate(tensorflow_lite)
    add_subdirectory(${tensorflow_lite_SOURCE_DIR}/tensorflow/lite ${tensorflow_lite_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Add PyTorch
FetchContent_Declare(
    pytorch
    GIT_REPOSITORY https://github.com/pytorch/pytorch.git
    GIT_TAG v2.2.1
)
FetchContent_GetProperties(pytorch)
if(NOT pytorch_POPULATED)
    FetchContent_Populate(pytorch)
    set(BUILD_SHARED_LIBS ON)
    set(BUILD_PYTHON OFF)
    set(BUILD_TEST OFF)
    set(BUILD_CAFFE2 OFF)
    set(USE_CUDA OFF)
    set(USE_METAL OFF)
    set(USE_VULKAN OFF)
    set(USE_OPENMP OFF)
    set(USE_MKLDNN OFF)
    set(USE_NNPACK OFF)
    set(USE_QNNPACK OFF)
    set(USE_XNNPACK OFF)
    add_subdirectory(${pytorch_SOURCE_DIR} ${pytorch_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Add ONNX Runtime
FetchContent_Declare(
    onnxruntime
    GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
    GIT_TAG v1.17.1
)
FetchContent_GetProperties(onnxruntime)
if(NOT onnxruntime_POPULATED)
    FetchContent_Populate(onnxruntime)
    set(ONNXRUNTIME_BUILD_SHARED_LIB ON)
    set(ONNXRUNTIME_BUILD_UNIT_TESTS OFF)
    set(ONNXRUNTIME_USE_CUDA OFF)
    set(ONNXRUNTIME_USE_VULKAN OFF)
    set(ONNXRUNTIME_USE_METAL OFF)
    set(ONNXRUNTIME_USE_COREML OFF)
    add_subdirectory(${onnxruntime_SOURCE_DIR} ${onnxruntime_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Add source files
file(GLOB_RECURSE SOURCES
    "core/*.cpp" "core/*.h"
    "conversion/*.cpp" "conversion/*.h"
    "hardware/*.cpp" "hardware/*.h"
    "inference/*.cpp" "inference/*.h"
    "monitoring/*.cpp" "monitoring/*.h"
    "benchmark/*.cpp" "benchmark/*.h"
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
    ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include
    ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/c++/v1
    ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/android
    ${tensorflow_lite_SOURCE_DIR}
    ${pytorch_SOURCE_DIR}
    ${onnxruntime_SOURCE_DIR}/include
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
    tensorflow-lite
    torch
    onnxruntime
    nlohmann_json::nlohmann_json
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/lib/${ANDROID_ABI}/libonnxruntime.so
)