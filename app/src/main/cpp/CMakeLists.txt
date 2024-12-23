cmake_minimum_required(VERSION 3.18.1)

project("mobileai")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find jsoncpp package
find_package(jsoncpp REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
)

# Add source files
add_library(${CMAKE_PROJECT_NAME} SHARED
    hardware/mtk_accelerator.cpp
    hardware/qualcomm_accelerator.cpp
    inference/model_engine.cpp
    core/hardware_manager.cpp
    monitoring/performance_monitor.cpp
)

# Find required packages
find_library(log-lib log)
find_library(android-lib android)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${log-lib}
    ${android-lib}
    JsonCpp::JsonCpp
)

# Add JsonCpp library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

# Add include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${JSONCPP_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware
)

# Link against JsonCpp
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    ${JSONCPP_LIBRARIES}
) 