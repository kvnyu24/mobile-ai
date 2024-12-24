cmake_minimum_required(VERSION 3.18.1)

project("mobileai")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find jsoncpp package
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Add TensorFlow Lite
set(TFLITE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tensorflow)
include_directories(
    ${TFLITE_DIR}/include
)

# Add TensorFlow Lite library
add_library(tensorflow-lite SHARED IMPORTED)
set_target_properties(tensorflow-lite PROPERTIES IMPORTED_LOCATION
    ${TFLITE_DIR}/lib/${ANDROID_ABI}/libtensorflowlite.so)

# Add source files
add_library(${CMAKE_PROJECT_NAME} SHARED
    hardware/mtk_accelerator.cpp
    hardware/qualcomm_accelerator.cpp
    inference/model_engine.cpp
    core/error_handler.cpp
)

# Set include directories for the target
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${JSONCPP_INCLUDE_DIRS}
    ${ANDROID_NDK}/sysroot/usr/include
    ${TFLITE_DIR}/include
)

# Find required packages
find_library(log-lib log)
find_library(android-lib android)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    ${log-lib}
    ${android-lib}
    ${JSONCPP_LIBRARIES}
    tensorflow-lite
) 

# Add compiler definitions
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ANDROID_PLATFORM
    USE_NEON
    USE_OPENMP
) 

# JsonCpp Configuration
include(ExternalProject)
ExternalProject_Add(jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.5
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external
        -DJSONCPP_WITH_TESTS=OFF
        -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF
    PREFIX ${CMAKE_BINARY_DIR}/external
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/external/lib/libjsoncpp.a
)

# Add JsonCpp include directory
include_directories(${CMAKE_BINARY_DIR}/external/include)
link_directories(${CMAKE_BINARY_DIR}/external/lib)

# Update include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tensorflow/include
    ${ANDROID_NDK}/sources/android/log/include
)

# Add NeuroPilot SDK if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/neuropilot)
    set(NEUROPILOT_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/neuropilot)
    include_directories(
        ${NEUROPILOT_SDK_DIR}/include
    )
endif()

# Add NeuroPilot library
add_library(neuropilot SHARED IMPORTED)
set_target_properties(neuropilot PROPERTIES IMPORTED_LOCATION
    ${NEUROPILOT_SDK_DIR}/lib/${ANDROID_ABI}/libneuropilot.so)

# Update target link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    android
    log
    jsoncpp_lib
    tensorflow-lite
    neuropilot
) 