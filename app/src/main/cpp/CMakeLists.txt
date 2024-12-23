cmake_minimum_required(VERSION 3.18.1)

project("mobileai")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_library(${CMAKE_PROJECT_NAME} SHARED
    hardware/mtk_accelerator.cpp
    hardware/qualcomm_accelerator.cpp
    inference/model_engine.cpp
    core/hardware_manager.cpp
)

# Find required packages
find_library(log-lib log)
find_library(android-lib android)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${log-lib}
    ${android-lib}
) 