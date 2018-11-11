project("secure_vision")

cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(OPENCL_GEN_DIR "/tmp/secure_vision/opencl")
set(WAYLAND_PROTO_DIR  ${PROJECT_SOURCE_DIR}/protocols)
set(XDG_SHELL_VERSION xdg-shell-unstable-v6)
set(XDG_SHELL_PROTO unstable/xdg-shell/${XDG_SHELL_VERSION})

include_directories(SYSTEM src/common ${OPENCL_GEN_DIR} ${WAYLAND_PROTO_DIR})

file(GLOB SOURCES "src/*.cc" "src/capture/*.cc" "src/synth/*.cc" ${WAYLAND_PROTO_DIR}/"*.c")
link_directories("/opt/intel/opencl")
add_custom_target(generate_wayland_proto_header COMMAND ./genproto client-header ${XDG_SHELL_PROTO}.xml ${WAYLAND_PROTO_DIR}/${XDG_SHELL_VERSION}-client-protocol.h)
add_custom_target(generate_wayland_proto_code COMMAND ./genproto private-code ${XDG_SHELL_PROTO}.xml ${WAYLAND_PROTO_DIR}/${XDG_SHELL_VERSION}-protocol.c)

add_custom_target(generate_opencl_headers COMMAND ./genincludes src/opencl ${OPENCL_GEN_DIR} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(secure_vision ${SOURCES})
add_dependencies(secure_vision
  generate_wayland_proto_header
  generate_wayland_proto_code
  generate_opencl_headers)

target_link_libraries(secure_vision stdc++fs wayland-cursor wayland-client wayland-egl EGL GLESv2 pthread GL OpenCL)
set_property(TARGET secure_vision PROPERTY COMPILE_FLAGS "-Wall -pedantic -Wextra -Wunreachable-code")

add_executable(png2dat tools/png2dat/main.cc)
target_link_libraries(png2dat png)
