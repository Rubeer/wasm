

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


global u8 __FrameMemory[1024*1024*8];
global memory_arena FrameMemory = MemoryArenaFromByteArray(__FrameMemory);

import_from_js void JS_GetFontAtlas(void *Pixels, size PixelsSize, void *Geometry, size GeomSize);

function void LoadFontAtlas(memory_arena *Memory, font_atlas *FontAtlas)
{
    u32 Width = 256;
    u32 Height = 256;
    u32 PixelCount = Width*Height;

    size PixelsSize = PixelCount*4;
    size GeomPackedSize = sizeof(font_atlas_char_packed) * 256;

    auto Pixels = (u8 *)PushSize(Memory, PixelsSize, ArenaFlag_NoClear);
    auto GeomPacked = (font_atlas_char_packed *)PushSize(Memory, GeomPackedSize, ArenaFlag_NoClear);

    JS_GetFontAtlas(Pixels, PixelsSize, GeomPacked, GeomPackedSize);

    f32 Scale = 1.0f / 256.0f;

    // TODO(robin): Benchmark if it isn't faster to just use the packed data as-is,
    // converting to float at runtime instead of ahead of time.
    for(u32 i = 0; i < 256; ++i)
    {
        font_atlas_char_packed C = GeomPacked[i];
        font_atlas_char *Char = FontAtlas->Geometry + i;

        Char->Min     = Scale * v2{(f32)C.MinX, (f32)C.MinY};
        Char->Max     = Scale * v2{(f32)C.MaxX, (f32)C.MaxY};
        Char->Offset  = Scale * v2{(f32)C.OffX, (f32)C.OffY};
        Char->Advance = Scale * (f32)C.Advance;
        Char->Offset.y = -(Char->Offset.y + Char->Max.y - Char->Min.y); // NOTE(robin): Flip Y
    }

    u32 *Pixels32 = (u32 *)Pixels;
    u8 *Pixels8 = (u8 *)Pixels;
    for(u32 i = 0; i < PixelCount; ++i)
    {
        // NOTE(robin): Only want the Alpha channel (the rest is all 255 anyway)
        Pixels8[i] = (u8)(Pixels32[i] >> 24);
    }

    FontAtlas->Texture = glCreateTexture();
    glBindTexture(GL_TEXTURE_2D, FontAtlas->Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, Width, Height, 0, GL_RED, GL_UNSIGNED_BYTE, Pixels, PixelsSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if 0
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
#endif
}


function void
Flush(draw_buffer *Buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, Buffer->VertexBuffer);
    glBindVertexArray(Buffer->VertexArray);
    glBufferSubData(GL_ARRAY_BUFFER, 0, Buffer->VertexCount*sizeof(Buffer->Vertices[0]), Buffer->Vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer->IndexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, Buffer->IndexCount*sizeof(Buffer->Indices[0]), Buffer->Indices);

    glUseProgram(Buffer->Program);
    glDrawElements(GL_TRIANGLES, Buffer->IndexCount, GL_UNSIGNED_SHORT, 0);

    Buffer->IndexCount = 0;
    Buffer->VertexCount = 0;
}


struct target_vertices_indices
{
    vertex *V;
    u16 *I;
    u16 FirstIndex;
};

function target_vertices_indices
AllocateVerticesAndIndices(draw_buffer *Buffer, u32 RequestedVertexCount, u32 RequestedIndexCount)
{
    if(Buffer->VertexCount + RequestedVertexCount >= ArrayCount(Buffer->Vertices) ||
       Buffer->IndexCount + RequestedIndexCount >= ArrayCount(Buffer->Indices)) 
    {
        Flush(Buffer);
    }

    target_vertices_indices Target;
    Target.V = Buffer->Vertices + Buffer->VertexCount;
    Target.I = Buffer->Indices + Buffer->IndexCount;
    Target.FirstIndex = (u16)Buffer->VertexCount;

    Buffer->VertexCount += RequestedVertexCount;
    Buffer->IndexCount += RequestedIndexCount;

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


function void PushBox(m3x4 const &Transform)
{

//		   .4------5     4------5     4------5     4------5     4------5.
//		 .' |    .'|    /|     /|     |      |     |\     |\    |`.    | `.
//		0---+--1'  |   0-+----1 |     0------1     | 0----+-1   |  `0--+---1
//		|   |  |   |   | |    | |     |      |     | |    | |   |   |  |   |
//		|  ,6--+---7   | 6----+-7     6------7     6-+----7 |   6---+--7   |
//		|.'    | .'    |/     |/      |      |      \|     \|    `. |   `. |
//		2------3'      2------3       2------3       2------3      `2------3
//

    target_vertices_indices Target = AllocateVerticesAndIndices(&State.Default, 8, 6*6);

    Target.V[0].P = Transform * v3{-1, -1,  1};
    Target.V[1].P = Transform * v3{ 1, -1,  1};
    Target.V[2].P = Transform * v3{-1, -1, -1};
    Target.V[3].P = Transform * v3{ 1, -1, -1};

    Target.V[4].P = Transform * v3{-1,  1,  1};
    Target.V[5].P = Transform * v3{ 1,  1,  1};
    Target.V[6].P = Transform * v3{-1,  1, -1};
    Target.V[7].P = Transform * v3{ 1,  1, -1};

    Target.V[0].C = Red;
    Target.V[1].C = Green;
    Target.V[2].C = Blue;
    Target.V[3].C = White;
    Target.V[4].C = Red;
    Target.V[5].C = Green;
    Target.V[6].C = Blue;
    Target.V[7].C = White;

    // TODO(robin): Triangle strip?
    PushQuadIndices(&Target, 0, 1, 2, 3); // Front
    PushQuadIndices(&Target, 1, 5, 3, 7); // Right
    PushQuadIndices(&Target, 4, 0, 6, 2); // Left
    PushQuadIndices(&Target, 4, 5, 0, 1); // Top
    PushQuadIndices(&Target, 2, 3, 6, 7); // Bottom
    PushQuadIndices(&Target, 5, 4, 7, 6); // Back
}

function void
PushText(string Text, m3x4 const &Transform = IdentityMatrix3x4)
{
    v2 TextAdvance = {};

    font_atlas_char *Geometry = State.FontAtlas.Geometry;

    target_vertices_indices Target = AllocateVerticesAndIndices(&State.Text, Text.Size*4, Text.Size*6);

    //v3 XAxis = GetXAxis(Transform);
    //v3 YAxis = GetYAxis(Transform);
    //v3 ZAxis = GetZAxis(Transform);
    //v3 Offset = GetTranslation(Transform);
    //v3 z = XAxis*1000.0f;
    //Printf("x %d y %d z %d", (s32)z.x, (s32)z.y, (s32)z.z);

    for(size i = 0; i < Text.Size; ++i)
    {
        char Char = Text.Contents[i];
        if(Char == '\n')
        {
            TextAdvance.x = 0;
            TextAdvance.y -= FONT_GLYPH_SIZE / 256.0f;
            State.Text.IndexCount -= 6;
            State.Text.VertexCount -= 4;
            continue;
        }

        font_atlas_char Geom = Geometry[(u8)Text.Contents[i]];

        v2 Min = Geom.Offset + TextAdvance;
        TextAdvance.x += Geom.Advance;
        v2 Dim = Geom.Max - Geom.Min;
        v2 Max = Min + Dim;

        Target.V[0].P = Transform * v3{Min.x, 0, Max.y};
        Target.V[1].P = Transform * v3{Max.x, 0, Max.y};
        Target.V[2].P = Transform * v3{Min.x, 0, Min.y};
        Target.V[3].P = Transform * v3{Max.x, 0, Min.y};

        v2 UVMin = Geom.Min;// * UVScale;
        v2 UVMax = Geom.Max;// * UVScale;
        Target.V[0].UV = UVMin;
        Target.V[1].UV = {UVMax.x, UVMin.y};
        Target.V[2].UV = {UVMin.x, UVMax.y};
        Target.V[3].UV = UVMax;

        Target.V[0].C = White;
        Target.V[1].C = White;
        Target.V[2].C = White;
        Target.V[3].C = White;

        PushQuadIndices(&Target, 0, 1, 2, 3);

        Target.V += 4;
        Target.FirstIndex += 4;

    }
}

function void InitDrawBuffer(draw_buffer *Buffer)
{
    Buffer->VertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Buffer->VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Buffer->Vertices), 0, GL_STREAM_DRAW);

    Buffer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Buffer->VertexArray);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, P));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, UV));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (void *)OffsetOf(vertex, C));


    Buffer->IndexBuffer = glCreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer->IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Buffer->Indices), 0, GL_STREAM_DRAW);
}

export_to_js void Init()
{
    string ShaderHeader = S(R"HereDoc(#version 300 es
precision highp float;
vec3 LinearToSRGB_Approx(vec3 x)
{
    // TODO(robin): Automatically disable this if the browser's framebuffer is sRGB!
    // https://mimosa-pudica.net/fast-gamma/
    vec3 a = vec3(0.00279491f);
    vec3 b = vec3(1.15907984f);
    vec3 c = vec3(0.157664895f);
    vec3 Result = (b * inversesqrt(x + a) - c) * x;
    return Result;
})HereDoc");



    string Vert = Concat(&FrameMemory, ShaderHeader, S(R"HereDoc(
layout (location=0) in vec3 Position;
layout (location=1) in vec2 UV;
layout (location=2) in vec4 Color;

out vec4 VertColor;

uniform mat4 Transform;

void main()
{
    VertColor = Color;
    gl_Position = Transform*vec4(Position, 1.0f);
})HereDoc"));



    string Frag = Concat(&FrameMemory, ShaderHeader, S(R"HereDoc(

in vec4 VertColor;
out vec4 FragColor;

void main()
{
    FragColor = VertColor;
    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
})HereDoc"));

    State.Default.Program = JS_GL_CreateCompileAndLinkProgram(Vert, Frag);
    State.Default.Transform = glGetUniformLocation(State.Default.Program, S("Transform"));


    Vert = Concat(&FrameMemory, ShaderHeader, S(R"HereDoc(
uniform mat4 Transform;

layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec4 Color;

out vec2 VertUV;
out vec4 VertColor;

void main()
{
    VertUV = UV;
    VertColor = Color;
    gl_Position = Transform * vec4(Position, 1.0f);
})HereDoc"));


    Frag = Concat(&FrameMemory, ShaderHeader, S(R"HereDoc(
uniform sampler2D TextureSampler;

in vec2 VertUV;
in vec4 VertColor;

out vec4 FragColor;

const float EdgeSmoothAmount = 2.0f; // In pixels
const vec3 OutlineColor = vec3(0.0f);
const float BorderThickness = 0.25;

const float OutlineBegin = 0.5f;
const float OutlineEnd  = OutlineBegin - BorderThickness;

float SmoothEdge(float Border, float Norm, float Distance)
{
    float Smoothing = Border*Norm;
    float Result = smoothstep(Border - Smoothing, Border + Smoothing, Distance);
    return Result;
}

void main()
{
    float Distance = texture(TextureSampler, VertUV).r;

    vec2 Derivative = vec2(dFdx(Distance), dFdy(Distance));
    float Norm = EdgeSmoothAmount * length(Derivative);


    vec3 Color = mix(OutlineColor, VertColor.rgb, SmoothEdge(OutlineBegin, Norm, Distance));

    FragColor = vec4(Color, VertColor.a*SmoothEdge(OutlineEnd, Norm, Distance));
    if(FragColor.a < 0.001f) discard;

    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
    FragColor.rgb *= FragColor.a;
})HereDoc"));

    State.Text.Program = JS_GL_CreateCompileAndLinkProgram(Vert, Frag);
    State.Text.Transform = glGetUniformLocation(State.Text.Program, S("Transform"));
    State.FontAtlas.TextureSampler = glGetUniformLocation(State.Text.Program, S("TextureSampler"));

    LoadFontAtlas(&FrameMemory, &State.FontAtlas);

    glUseProgram(State.Text.Program);
    glBindTexture(GL_TEXTURE_2D, State.FontAtlas.Texture);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i((GLint)State.FontAtlas.TextureSampler, 0);

    InitDrawBuffer(&State.Text);
    InitDrawBuffer(&State.Default);

    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

}

export_to_js void UpdateAndRender(u32 Width, u32 Height, f32 DeltaTime)
{
    v2 MousePixels = UserInput.MousePosPixels;
    v2 RenderDim = v2{(f32)Width, (f32)Height};
    v2 MouseClipSpace = 2.0f*(MousePixels - 0.5f*RenderDim) / RenderDim;
    MouseClipSpace.y = -MouseClipSpace.y;


    local_persist f32 Anim[4] = {0, 0.15f, 0.415f, 0.865f};
    f32 Speeds[4] = {0.001f, 0.1f, 0.1f, 0.1f};
    f32 Curve[ArrayCount(Anim)];
    for(size i = 0; i < ArrayCount(Anim); ++i)
    {
        if(Anim[i] >= 1.0f)
        {
            Anim[i] = 0.0f;
        }
        Curve[i] = SmoothCurve010(Anim[i]);
        Anim[i] += DeltaTime*Speeds[i];
    }

    v3 CameraP = {0, -107, 6};

    v3 Up = {0, 0, 1};

    v3 CameraZ = Normalize(CameraP);
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
    glUseProgram(State.Text.Program);
    glUniformMatrix4fv(State.Text.Transform, true, &RenderTransform.Forward);
    glUseProgram(State.Default.Program);
    glUniformMatrix4fv(State.Default.Transform, true, &RenderTransform.Forward);

    f32 Distance = 10.0f;
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

    random_state Random = DefaultSeed();

    u32 Count = 4000;
    for(u32 i = 0; i < Count; ++i)
    {
        f32 V = i / (f32)Count;

        v3 P;
        P.x = 7.0f*RandomBilateral(&Random) + 100.0f*CosineApproxN(V+Anim[0]);
        P.y = 7.0f*RandomBilateral(&Random) + 100.0f*SineApproxN(V+Anim[0]);

        P.z = CosineApproxN(V + RandomBilateral(&Random)) * 4.0f;

        v3 NormalDim = v3{0.9f, 0.9f, 0.9f};

        v3 WeirdDim = v3{0.6f, 0.3f, 0.3f};
        WeirdDim.x += 0.2f*RandomUnilateral(&Random);
        WeirdDim.y += 0.9f*RandomUnilateral(&Random);
        WeirdDim.z += 0.9f*RandomUnilateral(&Random);

        v3 Dim = Lerp(NormalDim, Curve[0], WeirdDim);
        v3 HalfDim = Dim*0.5f;

        m3x4 Rot = XRotationN(RandomUnilateral(&Random) + V + Anim[0]) * 
                   YRotationN(RandomUnilateral(&Random) + V + Anim[1]) *
                   ZRotationN(RandomUnilateral(&Random) + V + Anim[2]);

        m3x4 BoxTransform = Translation(P) * Rot * Scaling(HalfDim);
        PushBox(BoxTransform);

        if(LengthSquared(CameraP - P) < Square(20.0f))
        {
            m3x4 TextTransform = Translation(P) * Rot * Translation(-HalfDim.x, -HalfDim.y*1.002f, HalfDim.z) * Scaling(1.0f);
            char Buf[128];
            string Text = FormatText(Buf, "Positie\n"
                                          "x %d\n"
                                          "y %d\n"
                                          "z %d\n",
                                          (s32)(100.0f*P.x),
                                          (s32)(100.0f*P.y),
                                          (s32)(100.0f*P.z));
            PushText(Text, TextTransform);
        }
    }



    Flush(&State.Default);
    Flush(&State.Text);

    Reset(&FrameMemory);
}


