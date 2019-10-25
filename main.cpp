

#include "stuff.h"
#include "webgl2_defs.h"

GLuint CompileShader(string Source, GLenum Type)
{
    GLuint Shader = glCreateShader(Type);
    glShaderSource(Shader, Source.Size, Source.Contents);
    glCompileShader(Shader);
    return Shader;
}

struct v3
{
    f32 x,y,z;
};

struct vertex
{
    v3 P;
    u32 C;
};

js_export void Init(size InitMemorySize)
{
    GLuint Vert = CompileShader(S(
R"raw(  #version 300 es
        
        layout (location=0) in vec4 Position;
        layout (location=1) in vec3 Color;
        
        out vec3 VertColor;

        void main()
        {
            VertColor = Color;
            gl_Position = Position;
        }
    )raw"), GL_VERTEX_SHADER);

    GLuint Frag = CompileShader(S(
R"raw(  #version 300 es
        precision highp float;
        
        in vec3 VertColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(VertColor, 1.0);
        }
    )raw"), GL_FRAGMENT_SHADER);

    GLuint Program = glCreateProgram();
    glAttachShader(Program, Vert);
    glAttachShader(Program, Frag);
    glLinkProgram(Program);
    glUseProgram(Program);

    GLuint VertexArray = glCreateVertexArray();
    glBindVertexArray(VertexArray);

    GLuint Buffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Buffer);

    glVertexAttribPointer(0, 3, GL_FLOAT,         false, sizeof(vertex), (void *)offsetof(vertex, P));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, sizeof(vertex), (void *)offsetof(vertex, C));
    glEnableVertexAttribArray(1);

}

js_export void UpdateAndRender(u32 Width, u32 Height, f32 Test)
{
    f32 Triangle[3];
    Triangle[0].P = {-0.5f, -0.5f, 0.0f};
    Triangle[1].P = {0.5f, -0.5f, 0.0f};
    Triangle[2].P = {0.0f, 0.5f, 0.0f};
}
