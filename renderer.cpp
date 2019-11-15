
function void InitCommonDrawBuffers(renderer_common *Renderer)
{
    Renderer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Renderer->VertexArray);

    Renderer->VertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Renderer->Vertices), 0, GL_STREAM_DRAW);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, P));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, N));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, UV));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (void *)OffsetOf(vertex, C));


    Renderer->IndexBuffer = glCreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Renderer->Indices), 0, GL_STREAM_DRAW);
}

global string CommonShaderHeader = S(R"HereDoc(#version 300 es
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
}

float Square(float x) {return x*x;}
)HereDoc");

import_from_js void JS_GetFontAtlas(void *Pixels, size PixelsSize, void *Geometry, size GeomSize);
function void InitTextRendering(memory_arena *Memory, renderer_text *TextRenderer)
{
    InitCommonDrawBuffers(&TextRenderer->Common);

    // TOOD(robin): Don't hardcode the sizes
    u32 AtlasWidth = 256;
    u32 AtlasHeight = 256;
    u32 PixelCount = AtlasWidth*AtlasHeight;

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
        font_atlas_char *Char = TextRenderer->Geometry + i;

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

    TextRenderer->Texture = glCreateTexture();
    glBindTexture(GL_TEXTURE_2D, TextRenderer->Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AtlasWidth, AtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, Pixels, PixelsSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



    string Vert = Concat(Memory, CommonShaderHeader, S(R"HereDoc(
uniform mat4 Transform;

layout (location=0) in vec3 Position;
layout (location=1) in vec3 Normal;
layout (location=2) in vec2 UV;
layout (location=3) in vec4 Color;

out vec2 VertUV;
out vec4 VertColor;

void main()
{
    VertUV = UV;
    VertColor = Color;
    gl_Position = Transform * vec4(Position, 1.0f);
}
)HereDoc"));


    string Frag = Concat(Memory, CommonShaderHeader, S(R"HereDoc(
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
    if(FragColor.a < 0.00001f) discard;

    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
    FragColor.rgb *= FragColor.a;
}
)HereDoc"));

    TextRenderer->Common.Program = JS_GL_CreateCompileAndLinkProgram(Vert, Frag);
    TextRenderer->Common.Transform = glGetUniformLocation(TextRenderer->Common.Program, S("Transform"));
    TextRenderer->TextureSampler = glGetUniformLocation(TextRenderer->Common.Program, S("TextureSampler"));

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(TextRenderer->Common.Program);
    glUniform1i(TextRenderer->TextureSampler, 0);

}

function void InitDefaultRendering(memory_arena *Memory, renderer_default *DefaultRender)
{
    InitCommonDrawBuffers(&DefaultRender->Common);

    string Vert = Concat(Memory, CommonShaderHeader, S(R"HereDoc(
layout (location=0) in vec3 Position;
layout (location=1) in vec3 Normal;
layout (location=2) in vec2 UV;
layout (location=3) in vec4 Color;

out vec4 VertColor;

uniform mat4 Transform;
uniform vec3 MouseWorldP;

float LightFrom(vec3 LightPosition, float Brightness)
{
    vec3 ToLight = LightPosition - Position;
    float LengthSq = dot(ToLight, ToLight);

    float InvLengthSq = 1.0 / LengthSq;
    vec3 LightDir = ToLight * sqrt(InvLengthSq);

    float Factor = max(0.0, dot(Normal, LightDir));
    float LightStrength = Brightness * InvLengthSq;
    float Result = LightStrength * Factor*Factor;
    return Result;
}

void main()
{
    VertColor.a = Color.a;

    float SkyLight = LightFrom(vec3(0, 100.0, 300.0), 50000.0);
    float MouseLight = LightFrom(MouseWorldP, 40.0);

    VertColor.rgb = Color.rgb*min(1.0, SkyLight + MouseLight);

    gl_Position = Transform*vec4(Position, 1.0);
}
)HereDoc"));


    string Frag = Concat(Memory, CommonShaderHeader, S(R"HereDoc(
in vec4 VertColor;
out vec4 FragColor;

void main()
{
    FragColor = VertColor;
    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
}
)HereDoc"));

    DefaultRender->Common.Program = JS_GL_CreateCompileAndLinkProgram(Vert, Frag);
    DefaultRender->Common.Transform = glGetUniformLocation(DefaultRender->Common.Program, S("Transform"));
    DefaultRender->MouseWorldP = glGetUniformLocation(DefaultRender->Common.Program, S("MouseWorldP"));

}

function void
Flush(renderer_common *Renderer)
{
    glBindVertexArray(Renderer->VertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, Renderer->VertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, Renderer->VertexCount*sizeof(Renderer->Vertices[0]), Renderer->Vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, Renderer->IndexCount*sizeof(Renderer->Indices[0]), Renderer->Indices);

    glUseProgram(Renderer->Program);

    glDrawElements(GL_TRIANGLES, Renderer->IndexCount, GL_UNSIGNED_SHORT, 0);

    Renderer->IndexCount = 0;
    Renderer->VertexCount = 0;
}


function target_vertices_indices
AllocateVerticesAndIndices(renderer_common *Renderer, u32 RequestedVertexCount, u32 RequestedIndexCount)
{
    if(Renderer->VertexCount + RequestedVertexCount >= ArrayCount(Renderer->Vertices) ||
       Renderer->IndexCount + RequestedIndexCount >= ArrayCount(Renderer->Indices)) 
    {
        Flush(Renderer);
    }

    target_vertices_indices Target;
    Target.V = Renderer->Vertices + Renderer->VertexCount;
    Target.I = Renderer->Indices + Renderer->IndexCount;
    Target.FirstIndex = (u16)Renderer->VertexCount;

    Renderer->VertexCount += RequestedVertexCount;
    Renderer->IndexCount += RequestedIndexCount;

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

    Target->I[0] = Target->FirstIndex + V2;
    Target->I[1] = Target->FirstIndex + V1;
    Target->I[2] = Target->FirstIndex + V0;

    Target->I[3] = Target->FirstIndex + V1;
    Target->I[4] = Target->FirstIndex + V2;
    Target->I[5] = Target->FirstIndex + V3;

    Target->I += 6;
}
function void PushQuad_(target_vertices_indices *Target,
                        v3 P0, v3 P1, v3 P2,v3 P3,
                        u32 C0, u32 C1, u32 C2, u32 C3,
                        v3 Normal)
{
    Target->V[0] = {P0, Normal, {}, C0};
    Target->V[1] = {P1, Normal, {}, C1};
    Target->V[2] = {P2, Normal, {}, C2};
    Target->V[3] = {P3, Normal, {}, C3};
    PushQuadIndices(Target, 0, 1, 2, 3);
    Target->V += 4;
    Target->FirstIndex += 4;
}


function void PushBox(renderer_default *Renderer,
                      m3x4 const &Transform,
                      u32 C0 = Color32_Red,
                      u32 C1 = Color32_Green,
                      u32 C2 = Color32_Blue,
                      u32 C3 = Color32_White,
                      u32 C4 = Color32_Black,
                      u32 C5 = Color32_Red)
{

//		   .4------5     4------5     4------5     4------5     4------5.
//		 .' |    .'|    /|     /|     |      |     |\     |\    |`.    | `.
//		0---+--1'  |   0-+----1 |     0------1     | 0----+-1   |  `0--+---1
//		|   |  |   |   | |    | |     |      |     | |    | |   |   |  |   |
//		|  ,6--+---7   | 6----+-7     6------7     6-+----7 |   6---+--7   |
//		|.'    | .'    |/     |/      |      |      \|     \|    `. |   `. |
//		2------3'      2------3       2------3       2------3      `2------3
//

    target_vertices_indices Target = AllocateVerticesAndIndices(&Renderer->Common, 6*4, 6*6);

    v3 P0 = Transform * v3{-1, -1,  1};
    v3 P1 = Transform * v3{ 1, -1,  1};
    v3 P2 = Transform * v3{-1, -1, -1};
    v3 P3 = Transform * v3{ 1, -1, -1};
    v3 P4 = Transform * v3{-1,  1,  1};
    v3 P5 = Transform * v3{ 1,  1,  1};
    v3 P6 = Transform * v3{-1,  1, -1};
    v3 P7 = Transform * v3{ 1,  1, -1};

    v3 N0 = Normalize(TransformAs3x3(Transform, v3{ 0,-1, 0})); // Front
    v3 N1 = Normalize(TransformAs3x3(Transform, v3{ 1, 0, 0})); // Right
    v3 N2 = Normalize(TransformAs3x3(Transform, v3{-1, 0, 0})); // Left
    v3 N3 = Normalize(TransformAs3x3(Transform, v3{ 0, 0, 1})); // Top
    v3 N4 = Normalize(TransformAs3x3(Transform, v3{ 0, 0,-1})); // Bottom
    v3 N5 = Normalize(TransformAs3x3(Transform, v3{ 0, 1, 0})); // Back

    PushQuad_(&Target, P0, P1, P2, P3, C0, C0, C0, C0, N0); // Front
    PushQuad_(&Target, P1, P5, P3, P7, C1, C1, C1, C1, N1); // Right
    PushQuad_(&Target, P4, P0, P6, P2, C2, C2, C2, C2, N2); // Left
    PushQuad_(&Target, P4, P5, P0, P1, C3, C3, C3, C3, N3); // Top
    PushQuad_(&Target, P2, P3, P6, P7, C4, C4, C4, C4, N4); // Bottom
    PushQuad_(&Target, P5, P4, P7, P6, C5, C5, C5, C5, N5); // Back
}

function void
PushText(renderer_text *Renderer, string Text, m3x4 const &Transform = IdentityMatrix3x4)
{
    v2 TextAdvance = {};

    font_atlas_char *Geometry = Renderer->Geometry;

    target_vertices_indices Target = AllocateVerticesAndIndices(&Renderer->Common, Text.Size*4, Text.Size*6);

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
            Renderer->Common.IndexCount -= 6;
            Renderer->Common.VertexCount -= 4;
            continue;
        }

        font_atlas_char Geom = Geometry[(u8)Text.Contents[i]];

        v2 Min = Geom.Offset + TextAdvance;
        TextAdvance.x += Geom.Advance;
        v2 Dim = Geom.Max - Geom.Min;
        v2 Max = Min + Dim;

        Target.V[0].P = Transform * v3{Min.x, Max.y, 0};
        Target.V[1].P = Transform * v3{Max.x, Max.y, 0};
        Target.V[2].P = Transform * v3{Min.x, Min.y, 0};
        Target.V[3].P = Transform * v3{Max.x, Min.y, 0};

        v2 UVMin = Geom.Min;// * UVScale;
        v2 UVMax = Geom.Max;// * UVScale;
        Target.V[0].UV = UVMin;
        Target.V[1].UV = {UVMax.x, UVMin.y};
        Target.V[2].UV = {UVMin.x, UVMax.y};
        Target.V[3].UV = UVMax;

        Target.V[0].C = Color32_White;
        Target.V[1].C = Color32_White;
        Target.V[2].C = Color32_White;
        Target.V[3].C = Color32_White;

        PushQuadIndices(&Target, 0, 1, 2, 3);
        Target.V += 4;
        Target.FirstIndex += 4;

    }
}
