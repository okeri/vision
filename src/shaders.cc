#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <memory>
#include <functional>
#include <iostream>

#include "shaders.hh"

namespace shaders {

namespace {

using AttachFunc = std::function<void(GLuint,GLuint)>;
using AttachWraper = std::function<void(AttachFunc, GLuint)>;

GLuint compile(GLenum shaderType, const char* src) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint size;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
            if (size) {
                std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
                glGetShaderInfoLog(shader, size, NULL, buffer.get());
                std::cout << "Error: " << buffer.get() << std::endl;
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint program(AttachWraper wrapper) {
    GLuint program = glCreateProgram();
    if (program) {
        wrapper(glAttachShader, program);
        glLinkProgram(program);

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint size;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
            if (size) {
                std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
                glGetProgramInfoLog(program, size, NULL, buffer.get());
                std::cout << "Error: " << buffer.get() << std::endl;
            }
            wrapper(glDetachShader, program);
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

}  // namespace

GLuint program(const std::vector<ShaderInfo>& shaders) {
    std::vector<GLuint> compiled;
    for (const auto& shader : shaders) {
        compiled.emplace_back(compile(shader.type, shader.source));
    }
    return program([compiled] (AttachFunc func, GLuint p) {
            for (const auto &c : compiled) {
                func(p, c);
            }
        });
}

}  // namespace
