#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "shaders.hh"
#include "Renderer.hh"

#define GLSL(_X_) "#version 300 es\nprecision mediump float;\n" #_X_

const char* s_vert = GLSL(
in vec4 position;
out vec2 texCoord;

void main()
{
    texCoord = vec2(position[2],position[3]);
    gl_Position = vec4(position[0],position[1], 0, 1);
}
);

const char* s_frag = GLSL(
in vec2 texCoord;
out vec4 outColor;

uniform sampler2D frame;

void main()
{
    outColor = texture(frame, texCoord);
}
);

Renderer::Renderer() {
    GLuint texture_id;
    GLuint program = shaders::program({
            {GL_VERTEX_SHADER, s_vert},
            {GL_FRAGMENT_SHADER, s_frag}
        });

    static float vertices[] = {
        -1.f, 1.f, 1., 0.,
        -1.f, -1.f, 1., 1.,
        1.f,  -1.f, 0., 1.,

        1.f,  -1.f, 0., 1.,
        1.f,  1.f, 0., 0.,
        -1.f, 1.f, 1., 0.
    };


    glUseProgram(program);
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glActiveTexture(GL_TEXTURE0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices);
    glUseProgram(program);
}

void Renderer::render(const Frame &frame, const FrameInfo &info) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                 info.width, info.height, 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 frame.data());
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
