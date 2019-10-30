

#include "util.h"
#include "math.h"
#include "webgl2_defs.h"

#include "main.h"

global constexpr u32 White  = 0xFFFFFFFF;
global constexpr u32 Black  = 0xFF000000;
global constexpr u32 Red    = 0xFF0000FF;
global constexpr u32 Green  = 0xFF00FF00;
global constexpr u32 Blue   = 0xFFFF0000;


global state State;
global user_input UserInput;


export_to_js void MouseMove(s32 X, s32 Y)
{
    UserInput.MousePosPixels = {(f32)X, (f32)Y};
}

function void AddButtonTransition(button *Button, bool EndedDown)
{
    Button->EndedDown = EndedDown;
    Button->HalfTransitionCount++;
}

export_to_js void MouseLeft(bool EndedDown)
{
    AddButtonTransition(&UserInput.MouseLeft, EndedDown);
}

export_to_js void KeyPress(u32 KeyCode, bool EndedDown)
{
    if(KeyCode < ArrayCount(UserInput.Keys))
    {
        AddButtonTransition(&UserInput.Keys[KeyCode], EndedDown);
    }
}



function void
FlushDrawBuffers()
{
    opengl *OpenGL = &State.OpenGL;

    glBindBuffer(GL_ARRAY_BUFFER, OpenGL->VertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, State.VertexCount*sizeof(State.Vertices[0]), State.Vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->IndexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, State.IndexCount*sizeof(State.Indices[0]), State.Indices);

    glDrawElements(GL_TRIANGLES, State.IndexCount, GL_UNSIGNED_SHORT, 0);

    State.IndexCount = 0;
    State.VertexCount = 0;
}


struct target_vertices_indices
{
    vertex *V;
    u16 *I;
    u16 FirstIndex;
};

function target_vertices_indices
AllocateVerticesAndIndices(u32 RequestedVertexCount, u32 RequestedIndexCount)
{
    if(State.VertexCount + RequestedVertexCount >= ArrayCount(State.Vertices) ||
       State.IndexCount + RequestedIndexCount >= ArrayCount(State.Indices)) 
    {
        FlushDrawBuffers();
    }

    target_vertices_indices Target;
    Target.V = State.Vertices + State.VertexCount;
    Target.I = State.Indices + State.IndexCount;
    Target.FirstIndex = (u16)State.VertexCount;

    State.VertexCount += RequestedVertexCount;
    State.IndexCount += RequestedIndexCount;

    return Target;
}

function void PushQuadIndices(target_vertices_indices *Target,
                               u16 V0, u16 V1,
                               u16 V2, u16 V3)
{
//       0 _____1
//        |   /|
//        |  / |
//        | /  |
//        |/_ _|
//       2      3

    Target->I[0] = Target->FirstIndex + V0;
    Target->I[1] = Target->FirstIndex + V2;
    Target->I[2] = Target->FirstIndex + V1;

    Target->I[3] = Target->FirstIndex + V2;
    Target->I[4] = Target->FirstIndex + V3;
    Target->I[5] = Target->FirstIndex + V1;

    Target->I += 6;
}


function void PushBox(v3 Pos, v3 Dim)
{

//		   .4------5     4------5     4------5     4------5     4------5.
//		 .' |    .'|    /|     /|     |      |     |\     |\    |`.    | `.
//		0---+--1'  |   0-+----1 |     0------1     | 0----+-1   |  `0--+---1
//		|   |  |   |   | |    | |     |      |     | |    | |   |   |  |   |
//		|  ,6--+---7   | 6----+-7     6------7     6-+----7 |   6---+--7   |
//		|.'    | .'    |/     |/      |      |      \|     \|    `. |   `. |
//		2------3'      2------3       2------3       2------3      `2------3
//

    target_vertices_indices Target = AllocateVerticesAndIndices(8, 6*6);

    // TODO(robin): Triangle strip?

    v3 HalfDim = Dim*0.5f;
    v3 n = Pos - HalfDim;
    v3 p = Pos + HalfDim;

    Target.V[0].P = {n.x, n.y, p.z};
    Target.V[1].P = {p.x, n.y, p.z};
    Target.V[2].P = {n.x, n.y, n.z};
    Target.V[3].P = {p.x, n.y, n.z};

    Target.V[4].P = {n.x, p.y, p.z};
    Target.V[5].P = {p.x, p.y, p.z};
    Target.V[6].P = {n.x, p.y, n.z};
    Target.V[7].P = {p.x, p.y, n.z};

    Target.V[0].C = Red;
    Target.V[1].C = Green;
    Target.V[2].C = Blue;
    Target.V[3].C = White;
    Target.V[4].C = Red;
    Target.V[5].C = Green;
    Target.V[6].C = Blue;
    Target.V[7].C = White;

    PushQuadIndices(&Target, 0, 1, 2, 3); // Front
    PushQuadIndices(&Target, 1, 5, 3, 7); // Right
    PushQuadIndices(&Target, 4, 0, 6, 2); // Left
    PushQuadIndices(&Target, 4, 5, 0, 1); // Top
    PushQuadIndices(&Target, 2, 3, 6, 7); // Bottom
    PushQuadIndices(&Target, 5, 4, 7, 6); // Back
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(State.Vertices), 0, GL_STREAM_DRAW);

    OpenGL->IndexBuffer = glCreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(State.Indices), 0, GL_STREAM_DRAW);

    OpenGL->VertexArray = glCreateVertexArray();
    glBindVertexArray(OpenGL->VertexArray);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT,        false, sizeof(vertex), (void *)OffsetOf(vertex, P));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (void *)OffsetOf(vertex, C));

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}


export_to_js void UpdateAndRender(u32 Width, u32 Height, f32 DeltaTime)
{
    State.VertexCount = 0;
    State.IndexCount = 0;

    v2 MousePixels = UserInput.MousePosPixels;
    v2 RenderDim = v2{(f32)Width, (f32)Height};
    v2 MouseClipSpace = 2.0f*(MousePixels - 0.5f*RenderDim) / RenderDim;
    MouseClipSpace.y = -MouseClipSpace.y;


    local_persist f32 Anim[4] = {0, 0.15f, 0.415f, 0.865f};
    f32 Curve[ArrayCount(Anim)];
    for(size i = 0; i < ArrayCount(Anim); ++i)
    {
        if(Anim[i] >= 1.0f)
        {
            Anim[i] = 0.0f;
        }
        Curve[i] = SmoothCurve010(Anim[i]);
        Anim[i] += DeltaTime*0.1f;
    }

    v3 CameraP =
    {
        -3 + 6*Curve[0],
        -15,
        2 + 2*Curve[2],
    };

    v3 Up = {0, 0, 1};

    local_persist v3 CameraZ = Normalize(CameraP);
    //v3 CameraZ = Normalize(v3{0.2f,-1,0.2f});
    v3 CameraX = Normalize(Cross(Up, CameraZ));
    v3 CameraY = Cross(CameraZ, CameraX);
    m4x4_inv Camera = CameraTransform(CameraX, CameraY, CameraZ, CameraP);

    f32 NearClip = 0.1f;
    f32 FarClip = 1000.0f;
    f32 FocalLength = 1.0f;
    m4x4_inv Projection = PerspectiveProjectionTransform(Width, Height, NearClip, FarClip, FocalLength);

    m4x4_inv RenderTransform;
    RenderTransform.Forward = Projection.Forward * Camera.Forward;
    RenderTransform.Inverse = Camera.Inverse * Projection.Inverse;

    opengl *OpenGL = &State.OpenGL;
    glBindVertexArray(OpenGL->VertexArray);
    glUseProgram(OpenGL->Program);
    glUniformMatrix4fv(OpenGL->Projection, true, &RenderTransform.Forward);

    random_state Random = DefaultSeed();

    s32 MinX = -50;
    s32 MaxX =  50;
    s32 MinY = -14;
    s32 MaxY =  200;

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
            P.z = 1.5f*Curve[0]*RandomBilateral(&Random);
            P.z += Curve[0]*RandomBilateral(&Random) * 0.3f * (14.0f + P.y);

            v3 NormalDim = v3{0.8f, 0.8f, 0.8f};

            v3 WeirdDim = v3{0.2f, 0.2f, 0.2f};
            WeirdDim.x += 0.7f*RandomUnilateral(&Random);
            WeirdDim.y += 0.7f*RandomUnilateral(&Random);
            WeirdDim.z += 0.7f*RandomUnilateral(&Random);

            v3 Dim = Lerp(NormalDim, Curve[0], WeirdDim);

            PushBox(P, Dim);
        }
    }



    local_persist f32 Distance = 5.0f;
    {
        button *K = UserInput.Keys;
        if(K['W'].EndedDown)
        {
            Distance += 0.1f;
        }
        if(K['S'].EndedDown)
        {
            Distance -= 0.1f;
        }
    }


    v3 MouseWorldP = {};
    {

        v4 FromCamera = {0,0,0,1};
        FromCamera.xyz = CameraP - Distance*CameraZ;
        v4 FromCameraClip = RenderTransform.Forward * FromCamera;

        v4 ClipPos;
        ClipPos.xy = MouseClipSpace * FromCameraClip.w;
        ClipPos.zw = FromCameraClip.zw;

        MouseWorldP = (RenderTransform.Inverse * ClipPos).xyz;
    }


#if 0
    if(UserInput.MouseLeft.EndedDown)
        CameraZ = Normalize(CameraZ + (MouseWorldP - State.LastMouseWorldP));
    State.LastMouseWorldP = MouseWorldP;
#endif

    PushBox(MouseWorldP, v3{2,2,2});

    FlushDrawBuffers();
}


