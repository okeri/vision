#pragma once

#include <GL/gl.h>
#include <vector>

namespace shaders {

struct ShaderInfo {
    GLenum type;
    const char *source;
};

GLuint program(const std::vector<ShaderInfo>&);

}  // namespace
