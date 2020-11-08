project("secure_vision")

cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(OPENCL_GEN_DIR "/tmp/secure_vision/opencl")
set(WAYLAND_PROTO_DIR  ${PROJECT_SOURCE_DIR}/protocols)
set(WAYLAND_PROTO_IN_DIR "")
execute_process(COMMAND pkgconf --variable=pkgdatadir wayland-protocols OUTPUT_VARIABLE WAYLAND_PROTO_IN_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
if (WAYLAND_PROTO_IN_DIR STREQUAL "")
  message("Please install wayland-protocols pkgconf")
endif()

include_directories(SYSTEM src/common ${OPENCL_GEN_DIR} ${WAYLAND_PROTO_DIR})

add_custom_target(generate_wayland_proto_code COMMAND mkdir -p ${WAYLAND_PROTO_DIR} &&
  wayland-scanner client-header ${WAYLAND_PROTO_IN_DIR}/stable/xdg-shell/xdg-shell.xml ${WAYLAND_PROTO_DIR}/xdg-shell-client-protocol.h &&
  wayland-scanner private-code ${WAYLAND_PROTO_IN_DIR}/stable/xdg-shell/xdg-shell.xml ${WAYLAND_PROTO_DIR}/xdg-shell-protocol.c)

file(GLOB SOURCES "src/*.cc" "src/capture/*.cc" "src/synth/*.cc" ${WAYLAND_PROTO_DIR}/xdg-shell-protocol.c)
link_directories("/opt/intel/opencl")


add_custom_target(generate_opencl_headers COMMAND ./genincludes src/opencl ${OPENCL_GEN_DIR} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(secure_vision ${SOURCES})
add_dependencies(secure_vision
  generate_wayland_proto_code
  generate_opencl_headers)

target_link_libraries(secure_vision stdc++fs wayland-cursor wayland-client wayland-egl EGL GLESv2 pthread GL OpenCL)
set_property(TARGET secure_vision PROPERTY COMPILE_FLAGS "-Wall -pedantic -Wextra -Wunreachable-code")

add_executable(png2dat tools/png2dat/main.cc)
target_link_libraries(png2dat png)
