cmake_minimum_required(VERSION 3.18.1)

project("mobileai")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set third-party paths
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(TFLITE_DIR ${THIRD_PARTY_DIR}/tensorflow)
set(PYTORCH_DIR ${THIRD_PARTY_DIR}/pytorch)
set(ONNX_DIR ${THIRD_PARTY_DIR}/onnxruntime)
set(NEUROPILOT_DIR ${THIRD_PARTY_DIR}/neuropilot)
set(SNAPDRAGON_DIR ${THIRD_PARTY_DIR}/snapdragon)
set(JSONCPP_DIR ${THIRD_PARTY_DIR}/jsoncpp)

# Configure Android settings
if(ANDROID)
    set(ANDROID_PLATFORM android-26 CACHE STRING "Android platform version")
    set(ANDROID_STL c++_shared CACHE STRING "Android STL variant")
    if(NOT DEFINED ANDROID_ABI)
        set(ANDROID_ABI arm64-v8a CACHE STRING "Android ABI")
    endif()
endif()

# Enable NEON for ARM builds
if(ANDROID_ABI MATCHES "^armeabi-v7a|^arm64-v8a")
    set(CMAKE_ANDROID_ARM_NEON ON)
endif()

# Find required packages
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/conversion
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/inference
    ${CMAKE_CURRENT_SOURCE_DIR}/monitoring
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark
    ${TFLITE_DIR}/include
    ${PYTORCH_DIR}/include
    ${ONNX_DIR}/include
    ${NEUROPILOT_DIR}/include
    ${SNAPDRAGON_DIR}/include
    ${JSONCPP_DIR}/include
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

# Find required Android libraries
if(ANDROID)
    find_library(log-lib log)
    find_library(android-lib android)
    
    target_link_libraries(${CMAKE_PROJECT_NAME}
        PRIVATE
        ${log-lib}
        ${android-lib}
        tensorflow-lite
        pytorch_jni
        onnxruntime
        jsoncpp
        OpenMP::OpenMP_CXX
    )
endif()

# Add compiler definitions
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<BOOL:${ANDROID}>:ANDROID_PLATFORM>
    USE_NEON
    USE_OPENMP
)

# Add external libraries
add_library(tensorflow-lite SHARED IMPORTED)
set_target_properties(tensorflow-lite PROPERTIES IMPORTED_LOCATION
    ${TFLITE_DIR}/lib/${ANDROID_ABI}/libtensorflowlite.so)

add_library(pytorch_jni SHARED IMPORTED)
set_target_properties(pytorch_jni PROPERTIES IMPORTED_LOCATION
    ${PYTORCH_DIR}/lib/${ANDROID_ABI}/libtorch.so)

add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES IMPORTED_LOCATION
    ${ONNX_DIR}/lib/${ANDROID_ABI}/libonnxruntime.so)

add_library(jsoncpp SHARED IMPORTED)
set_target_properties(jsoncpp PROPERTIES IMPORTED_LOCATION
    ${JSONCPP_DIR}/lib/${ANDROID_ABI}/libjsoncpp.so)

# Print debug information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Android: ${ANDROID}")
if(ANDROID)
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
    message(STATUS "Android STL: ${ANDROID_STL}")
    message(STATUS "Android NDK: ${ANDROID_NDK}")
endif()