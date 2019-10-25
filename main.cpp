

#include "stuff.h"
#include "webgl2_defs.h"

struct v4
{
    f32 x,y,z,w;
};

struct vertex
{
    v4 Position;
    u32 Color;
};


js_export void Init(size InitMemorySize)
{
    (void)InitMemorySize;
    string Vert = S(
R"raw(  #version 300 es
        
        layout (location=0) in vec4 Position;
        layout (location=1) in vec4 Color;
        
        out vec4 VertColor;

        void main()
        {
            VertColor = Color;
            gl_Position = Position;
        }
    )raw");

    string Frag = S(
R"raw(  #version 300 es
        precision highp float;
        
        in vec4 VertColor;
        out vec4 FragColor;

        vec3 LinearToSRGB_Approx(vec3 x)
        {
            // NOTE(robin): The GPU has dedicated hardware to do this but we can't access that in WebGL..
            // https://mimosa-pudica.net/fast-gamma/
            vec3 a = vec3(0.00279491f);
            vec3 b = vec3(1.15907984f);
            vec3 c = b * inversesqrt(a + vec3(1.0f)) - vec3(1.0f);
            vec3 Result = (b * inversesqrt(x + a) - c) * x;
            return Result;
        }

        void main()
        {
            FragColor = vec4(LinearToSRGB_Approx(VertColor.rgb), VertColor.a);
            //FragColor = VertColor;
        }
    )raw");

    GLuint Program = JS_GL_CreateCompileAndLinkProgram(Vert.Size, Vert.Contents,
                                                       Frag.Size, Frag.Contents);
    glUseProgram(Program);

    GLuint Buffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Buffer);

    GLuint VertexArray = glCreateVertexArray();
    glBindVertexArray(VertexArray);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT,         false, sizeof(vertex), (void *)OffsetOf(vertex, Position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (void *)OffsetOf(vertex, Color));


}

js_export void UpdateAndRender(u32 Width, u32 Height, f32 Test)
{
    static f32 TestValue = -1.0f;
    TestValue += 0.005f;

    if(TestValue >= 1.0f)
    {
        TestValue -= 2.0f;
    }

    vertex Triangle[] =
    {
        {{-0.5f,     -0.5f, 0.0f, 1.0f}, 0xFF0000FF},
        {{ 0.5f,     -0.5f, 0.0f, 1.0f}, 0xFF00FF00},
        {{ TestValue, 0.5f, 0.0f, 1.0f}, 0xFFFF0000},
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(Triangle), Triangle, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
