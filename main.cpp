

#include "util.h"
#include "webgl2_defs.h"

global constexpr u32 White  = 0xFFFFFFFF;
global constexpr u32 Black  = 0xFF000000;
global constexpr u32 Red    = 0xFF0000FF;
global constexpr u32 Green  = 0xFF00FF00;
global constexpr u32 Blue   = 0xFFFF0000;

struct vertex
{
    v3 P;
    u32 C;
};


struct opengl
{
    GLuint VertexBuffer;
    GLuint VertexArray;
    GLuint Program;

    // Uniforms
    GLuint Projection;
};

struct state
{
    opengl OpenGL;

    u32 VertexCount;
    vertex Vertices[1 << 21];
};

global state State;

function void PushQuad(vertex V0,
                       vertex V1,
                       vertex V2,
                       vertex V3)
{
    vertex *Verts = State.Vertices + State.VertexCount;
    State.VertexCount += 6;

//       0 _____1
//        |   /|
//        |  / |
//        | /  |
//        |/_ _|
//       2      3
    // TOOD(robin): Index buffer
    Verts[0] = V0;
    Verts[1] = V2;
    Verts[2] = V1;

    Verts[3] = V2;
    Verts[4] = V3;
    Verts[5] = V1;
}

function void PushQuad(v3 P0, u32 C0,
                       v3 P1, u32 C1,
                       v3 P2, u32 C2,
                       v3 P3, u32 C3)
{
    PushQuad(vertex{P0,C0},
             vertex{P1,C1},
             vertex{P2,C2},
             vertex{P3,C3});
}

function void PushBox(v3 P, v3 Dim)
{
    f32 dx = Dim.x * 0.5f;
    f32 dy = Dim.y * 0.5f;
    f32 dz = Dim.z * 0.5f;

//		   .4------5     4------5     4------5     4------5     4------5.
//		 .' |    .'|    /|     /|     |      |     |\     |\    |`.    | `.
//		0---+--1'  |   0-+----1 |     0------1     | 0----+-1   |  `0--+---1
//		|   |  |   |   | |    | |     |      |     | |    | |   |   |  |   |
//		|  ,6--+---7   | 6----+-7     6------7     6-+----7 |   6---+--7   |
//		|.'    | .'    |/     |/      |      |      \|     \|    `. |   `. |
//		2------3'      2------3       2------3       2------3      `2------3
//
    vertex V0, V1, V2, V3, V4, V5, V6, V7;

    V0.P = P + v3{-dx,-dy, dz};
    V1.P = P + v3{ dx,-dy, dz};
    V2.P = P + v3{-dx,-dy,-dz};
    V3.P = P + v3{ dx,-dy,-dz};

    V4.P = P + v3{-dx, dy, dz};
    V5.P = P + v3{ dx, dy, dz};
    V6.P = P + v3{-dx, dy,-dz};
    V7.P = P + v3{ dx, dy,-dz};

    V0.C = Red;
    V1.C = Green;
    V2.C = Blue;
    V3.C = White;
    V4.C = Red;
    V5.C = Green;
    V6.C = Blue;
    V7.C = White;

    // TOOD(robin): Index buffer
    PushQuad(V0, V1, V2, V3); // Front
    PushQuad(V1, V5, V3, V7); // Right
    PushQuad(V4, V0, V6, V2); // Left
    PushQuad(V4, V5, V0, V1); // Top
    PushQuad(V2, V3, V6, V7); // Bottom
    PushQuad(V5, V4, V7, V6); // Back
}


export_to_js void InitOpenGL()
{
    string Vert = S(
R"raw(  #version 300 es
        
        layout (location=0) in vec3 Position;
        layout (location=1) in vec4 Color;
        
        out vec4 VertColor;

        uniform mat4 Projection;

        void main()
        {
            VertColor = Color;
            gl_Position = Projection*vec4(Position, 1.0f);
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
            vec3 c = vec3(0.157664895f);
            vec3 Result = (b * inversesqrt(x + a) - c) * x;
            return Result;
        }

        void main()
        {
            FragColor = VertColor;
            FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
        }
    )raw");

    opengl *OpenGL = &State.OpenGL;

    OpenGL->Program = JS_GL_CreateCompileAndLinkProgram(Vert.Size, Vert.Contents,
                                                       Frag.Size, Frag.Contents);

    OpenGL->Projection = glGetUniformLocation(OpenGL->Program, S("Projection"));

    OpenGL->VertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, OpenGL->VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, ArrayCount(State.Vertices)*sizeof(vertex), 0, GL_STREAM_DRAW);

    OpenGL->VertexArray = glCreateVertexArray();
    glBindVertexArray(OpenGL->VertexArray);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT,         false, sizeof(vertex), (void *)OffsetOf(vertex, P));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (void *)OffsetOf(vertex, C));

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);


}


export_to_js void UpdateAndRender(u32 Width, u32 Height, f32 DeltaTime)
{
    State.VertexCount = 0;

    //PushBox(v3{0,0,v*0.3f}, v3{5,5,5});

    static f32 t[4] = {0, 0.15f, 0.415f, 0.865f};
    f32 v[4];
    for(size i = 0; i < ArrayCount(t); ++i)
    {
        t[i] += DeltaTime*0.1f;
        if(t[i] >= 1.0f)
        {
            t[i] = 0.0f;
        }
        v[i] = SmoothCurve010(t[i]);
    }


    random_state Random = DefaultSeed();

    s32 MinX = -50;
    s32 MaxX =  50;
    s32 MinY = -14;
    s32 MaxY =  50;

    s32 VertsPerBox = 6*6;
    s32 TotalVerts = VertsPerBox * (MaxX - MinX) * (MaxY - MinY);
    Assert(State.VertexCount + (u32)TotalVerts < ArrayCount(State.Vertices));

    v3 LookAt = {};

    for(s32 y = MinY; y < MaxY; ++y)
    {
        for(s32 x = MinX; x < MaxX; ++x)
        {
            //
            // NOTE(robin): Should normally use instanced rendering, but this is just a test
            //

            v3 P;
            P.x = (f32)x;
            P.y = (f32)y;
            P.z = 1.5f*v[0]*RandomBilateral(&Random);

            v3 NormalDim = v3{0.8f, 0.8f, 0.8f};

            v3 WeirdDim = v3{0.2f, 0.2f, 0.2f};
            WeirdDim.x += 0.7f*RandomUnilateral(&Random);
            WeirdDim.y += 0.7f*RandomUnilateral(&Random);
            WeirdDim.z += 0.7f*RandomUnilateral(&Random);

            v3 Dim = Lerp(NormalDim, v[0], WeirdDim);

            PushBox(P, Dim);

            if(x == 0 && y == -10) LookAt = P;
        }
    }


    v3 CameraP = {-3 + 6*v[0], -15, 2 + 1*v[1]};

    v3 Up = {0, 0, 1};

    v3 CameraZ = Normalize(CameraP - LookAt);
    v3 CameraX = Normalize(Cross(Up, CameraZ));
    v3 CameraY = Cross(CameraZ, CameraX);
    m4x4_inv Camera = CameraTransform(CameraX, CameraY, CameraZ, CameraP);

    f32 NearClip = 0.1f;
    f32 FarClip = 100.0f;
    f32 FocalLength = 1.0f;
    m4x4_inv Projection = PerspectiveProjectionTransform(Width, Height, NearClip, FarClip, FocalLength);

    m4x4 Transform = Projection.Forward * Camera.Forward;

    opengl *OpenGL = &State.OpenGL;

    glBindBuffer(GL_ARRAY_BUFFER, OpenGL->VertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, State.VertexCount*sizeof(vertex), State.Vertices);

    glBindVertexArray(OpenGL->VertexArray);

    glUseProgram(OpenGL->Program);
    glUniformMatrix4fv(OpenGL->Projection, true, &Transform);

    glDrawArrays(GL_TRIANGLES, 0, State.VertexCount);
}


