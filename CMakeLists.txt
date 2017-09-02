project("secure_vision")

cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(OPENCL_GEN_DIR "/tmp/secure_vision/opencl")
include_directories(SYSTEM common ${OPENCL_GEN_DIR})

file(GLOB capture_SRC  "capture/*.cc")
add_library(capture ${capture_SRC})

file(GLOB synth_SRC  "synth/*.cc")
add_library(synth ${synth_SRC})

# file(GLOB common_SRC  "common/*.cc")
# add_library(common ${common_SRC})

file(GLOB vision_SRC "vision/*.cc")
link_directories("/opt/intel/opencl")

add_custom_target(generate_opencl_headers COMMAND ./genincludes opencl ${OPENCL_GEN_DIR} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(secure_vision ${vision_SRC})
add_dependencies(secure_vision generate_opencl_headers)
target_link_libraries(secure_vision capture synth stdc++fs wayland-cursor wayland-client wayland-egl EGL GLESv2 pthread GL OpenCL)
