
function void AllocateBuffers(memory_arena *Memory, renderer_common *Renderer, u32 MaxIndexCount, u32 MaxVertexCount = (1 << 16) - 1)
{
    Assert(MaxIndexCount >= MaxVertexCount);

    Renderer->MaxIndexCount = MaxIndexCount;
    Renderer->MaxVertexCount = MaxVertexCount;

    Renderer->Vertices = PushArray(Memory, vertex, MaxVertexCount, ArenaFlag_NoClear);
    Renderer->Indices = PushArray(Memory, u16, MaxIndexCount, ArenaFlag_NoClear);

    Renderer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Renderer->VertexArray);

    Renderer->VertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*Renderer->MaxVertexCount*sizeof(vertex), 0, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, P));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, N));
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, UV));
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (void *)OffsetOf(vertex, C));


    Renderer->IndexBuffer = glCreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Renderer->MaxIndexCount*sizeof(u16), 0, GL_STREAM_DRAW);
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
function void InitTextRendering(memory_arena *PermMem, memory_arena *TmpMem, renderer_text *TextRenderer)
{
    u32 MaxVertexCount = (1 << 16) - 1;
    u32 MaxIndexCount = (MaxVertexCount*6) / 4;
    AllocateBuffers(PermMem, &TextRenderer->Common, MaxIndexCount, MaxVertexCount);

    // TOOD(robin): Don't hardcode the sizes
    u32 AtlasWidth = 256;
    u32 AtlasHeight = 256;
    u32 PixelCount = AtlasWidth*AtlasHeight;

    size PixelsSize = PixelCount*4;
    size GeomPackedSize = sizeof(font_atlas_char_packed) * 256;

    auto Pixels = (u8 *)PushSize(TmpMem, PixelsSize, ArenaFlag_NoClear);
    auto GeomPacked = (font_atlas_char_packed *)PushSize(TmpMem, GeomPackedSize, ArenaFlag_NoClear);

    JS_GetFontAtlas(Pixels, PixelsSize, GeomPacked, GeomPackedSize);
    Printf("x");

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AtlasWidth, AtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, Pixels, PixelsSize/4, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);



    string VertexSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
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


    string FragmentSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
uniform sampler2D TextureSampler;

in vec2 VertUV;
in vec4 VertColor;

out vec4 FragColor;


float SmoothEdge(float Border, float Norm, float Distance)
{
    float Smoothing = Border*Norm;
    float Result = smoothstep(Border - Smoothing, Border + Smoothing, Distance);
    return Result;
}

void main()
{
    float Distance = texture(TextureSampler, VertUV).r;

    //vec2 Derivative = vec2(dFdx(Distance), dFdy(Distance));
    const float EdgeSmoothAmount = 1.5f;
    float Norm = EdgeSmoothAmount * fwidth(Distance);//length(Derivative);

    const vec3 OutlineColor = vec3(0.0);
    const float OutlineBegin = 0.5;
    float OutlineEnd  = OutlineBegin - 0.25;

    vec3 Color = mix(OutlineColor, VertColor.rgb, SmoothEdge(OutlineBegin, Norm, Distance));

    FragColor = vec4(Color, VertColor.a*SmoothEdge(OutlineEnd, Norm, Distance));
    if(FragColor.a < 0.00001f) discard;

    FragColor.rgb *= FragColor.a; // TODO(robin): Can we fix the blend mode so we don't need to do this?
    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
}
)HereDoc"));

    TextRenderer->Common.Program = JS_GL_CreateCompileAndLinkProgram(VertexSource, FragmentSource);
    TextRenderer->Common.Transform = glGetUniformLocation(TextRenderer->Common.Program, S("Transform"));
    TextRenderer->TextureSampler = glGetUniformLocation(TextRenderer->Common.Program, S("TextureSampler"));

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(TextRenderer->Common.Program);
    glUniform1i(TextRenderer->TextureSampler, 0);

}

function void InitDefaultRendering(memory_arena *PermMem, memory_arena *TmpMem, renderer_default *DefaultRender)
{
    u32 MaxVertexCount = (1 << 16) - 1;
    u32 MaxIndexCount = (MaxVertexCount*6) / 4;
    AllocateBuffers(PermMem, &DefaultRender->Common, MaxIndexCount, MaxVertexCount);

    string VertexSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
layout (location=0) in vec3 Position;
layout (location=1) in vec3 Normal;
layout (location=2) in vec2 UV;
layout (location=3) in vec4 Color;

uniform mat4 Transform;
uniform vec3 MouseWorldP;

out vec4 VertColor;

float LightFrom(vec3 LightPosition, float Brightness)
{
    vec3 ToLight = LightPosition - Position;
    float LightStrength = Brightness / dot(ToLight, ToLight);
    float Factor = max(0.0, dot(Normal, normalize(ToLight)));
    float Result = LightStrength * Factor*Factor;
    return Result;
}

void main()
{
    VertColor.a = Color.a;

    float SkyLight = LightFrom(vec3(0, 100.0, 300.0), 30000.0);
    float MouseLight = LightFrom(MouseWorldP, 40.0);

    VertColor.rgb = Color.rgb*min(1.0, SkyLight + MouseLight);

    gl_Position = Transform*vec4(Position, 1.0);
}
)HereDoc"));


    string FragmentSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
in vec4 VertColor;
out vec4 FragColor;

void main()
{
    FragColor = VertColor;
    FragColor.rgb *= FragColor.a; // TODO(robin): Can we fix the blend mode so we don't need to do this?
    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
}
)HereDoc"));

    DefaultRender->Common.Program = JS_GL_CreateCompileAndLinkProgram(VertexSource, FragmentSource);
    DefaultRender->Common.Transform = glGetUniformLocation(DefaultRender->Common.Program, S("Transform"));
    DefaultRender->MouseWorldP = glGetUniformLocation(DefaultRender->Common.Program, S("MouseWorldP"));

}

function void
Flush(renderer_common *Renderer)
{
    glBindVertexArray(Renderer->VertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, Renderer->VertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, Renderer->VertexCount*sizeof(vertex), Renderer->Vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, Renderer->IndexCount*sizeof(u16), Renderer->Indices);

    glUseProgram(Renderer->Program);

    glDrawElements(GL_TRIANGLES, Renderer->IndexCount, GL_UNSIGNED_SHORT, 0);

    Renderer->IndexCount = 0;
    Renderer->VertexCount = 0;
}


function target_vertices_indices
AllocateVerticesAndIndices(renderer_common *Renderer, u32 RequestedVertexCount, u32 RequestedIndexCount)
{
    if(Renderer->VertexCount + RequestedVertexCount > Renderer->MaxVertexCount ||
       Renderer->IndexCount + RequestedIndexCount > Renderer->MaxIndexCount) 
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
                      //v3 Pos, v3 Dim, quaternion Orient,
                      m3x4 const &Transform,
                      u32 C0 = Color32_White,
                      u32 C1 = Color32_White,
                      u32 C2 = Color32_White,
                      u32 C3 = Color32_White,
                      u32 C4 = Color32_White,
                      u32 C5 = Color32_White)
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

#if 1
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
#else
    v3 n = -0.5f*Dim;
    v3 p =  0.5f*Dim;
    v3 P0 = Rotate(v3{n.x, n.y, p.z}, Orient) + Pos;
    v3 P1 = Rotate(v3{p.x, n.y, p.z}, Orient) + Pos;
    v3 P2 = Rotate(v3{n.x, n.y, n.z}, Orient) + Pos;
    v3 P3 = Rotate(v3{p.x, n.y, n.z}, Orient) + Pos;
    v3 P4 = Rotate(v3{n.x, p.y, p.z}, Orient) + Pos;
    v3 P5 = Rotate(v3{p.x, p.y, p.z}, Orient) + Pos;
    v3 P6 = Rotate(v3{n.x, p.y, n.z}, Orient) + Pos;
    v3 P7 = Rotate(v3{p.x, p.y, n.z}, Orient) + Pos;

    v3 N0 = Rotate(v3{ 0,-1, 0}, Orient); // Front
    v3 N1 = Rotate(v3{ 1, 0, 0}, Orient); // Right
    v3 N2 = Rotate(v3{-1, 0, 0}, Orient); // Left
    v3 N3 = Rotate(v3{ 0, 0, 1}, Orient); // Top
    v3 N4 = Rotate(v3{ 0, 0,-1}, Orient); // Bottom
    v3 N5 = Rotate(v3{ 0, 1, 0}, Orient); // Back
#endif

    PushQuad_(&Target, P0, P1, P2, P3, C0, C0, C0, C0, N0); // Front
    PushQuad_(&Target, P1, P5, P3, P7, C1, C1, C1, C1, N1); // Right
    PushQuad_(&Target, P4, P0, P6, P2, C2, C2, C2, C2, N2); // Left
    PushQuad_(&Target, P4, P5, P0, P1, C3, C3, C3, C3, N3); // Top
    PushQuad_(&Target, P2, P3, P6, P7, C4, C4, C4, C4, N4); // Bottom
    PushQuad_(&Target, P5, P4, P7, P6, C5, C5, C5, C5, N5); // Back
}

function void
PushText(renderer_text *Renderer, string Text, m3x4 const &Transform = IdentityMatrix3x4, u32 Color = Color32_White)
{
    v2 TextAdvance = {};
    font_atlas_char *Geometry = Renderer->Geometry;
    target_vertices_indices Target = AllocateVerticesAndIndices(&Renderer->Common, Text.Size*4, Text.Size*6);

    for(size i = 0; i < Text.Size; ++i)
    {
        char Char = Text.Contents[i];
        if(Char == '\n')
        {
            TextAdvance.x = 0;
            TextAdvance.y -= FONT_GLYPH_SIZE / 256.0f;
            Renderer->Common.VertexCount -= 4;
            Renderer->Common.IndexCount -= 6;
            continue;
        }

        font_atlas_char Geom = Geometry[(u8)Char];

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

        Target.V[0].C = Color;
        Target.V[1].C = Color;
        Target.V[2].C = Color;
        Target.V[3].C = Color;

        PushQuadIndices(&Target, 0, 1, 2, 3);
        Target.V += 4;
        Target.FirstIndex += 4;

    }
}

function void
InitBoxRendering(memory_arena *PermMem, memory_arena *TmpMem, renderer_boxes *Renderer, u32 MaxInstanceCount = (1 << 16))
{
    Renderer->MaxInstanceCount = MaxInstanceCount;
    Renderer->InstanceData = PushArray(PermMem, box_instance, Renderer->MaxInstanceCount, ArenaFlag_NoClear);

    v3 P0 = {-1, -1,  1};
    v3 P1 = { 1, -1,  1};
    v3 P2 = {-1, -1, -1};
    v3 P3 = { 1, -1, -1};
    v3 P4 = {-1,  1,  1};
    v3 P5 = { 1,  1,  1};
    v3 P6 = {-1,  1, -1};
    v3 P7 = { 1,  1, -1};

    v3 N0 = { 0,-1, 0};
    v3 N1 = { 1, 0, 0};
    v3 N2 = {-1, 0, 0};
    v3 N3 = { 0, 0, 1};
    v3 N4 = { 0, 0,-1};
    v3 N5 = { 0, 1, 0};

    box_vertex Vertices[24];
    Vertices[0]  = {P0, N0};
    Vertices[1]  = {P1, N0};
    Vertices[2]  = {P2, N0};
    Vertices[3]  = {P3, N0};

    Vertices[4]  = {P1, N1};
    Vertices[5]  = {P5, N1};
    Vertices[6]  = {P3, N1};
    Vertices[7]  = {P7, N1};

    Vertices[8]  = {P4, N2};
    Vertices[9]  = {P0, N2};
    Vertices[10] = {P6, N2};
    Vertices[11] = {P2, N2};

    Vertices[12] = {P4, N3};
    Vertices[13] = {P5, N3};
    Vertices[14] = {P0, N3};
    Vertices[15] = {P1, N3};

    Vertices[16] = {P2, N4};
    Vertices[17] = {P3, N4};
    Vertices[18] = {P6, N4};
    Vertices[19] = {P7, N4};

    Vertices[20] = {P5, N5};
    Vertices[21] = {P4, N5};
    Vertices[22] = {P7, N5};
    Vertices[23] = {P6, N5};

    u16 Indices[36];
    u16 *Index = Indices;
    for(u16 Face = 0; Face < 6; ++Face)
    {
        Index[0] = 4*Face + 2;
        Index[1] = 4*Face + 1;
        Index[2] = 4*Face + 0;
        Index[3] = 4*Face + 1;
        Index[4] = 4*Face + 2;
        Index[5] = 4*Face + 3;
        //Printf("%u %u %u %u %u %u", Index[0], Index[1], Index[2], Index[3], Index[4], Index[5]);

        Index += 6;
    }

    Renderer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Renderer->VertexArray);

    Renderer->CubeShapeIndexBuffer = glCreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->CubeShapeIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

    Renderer->CubeShapeVertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->CubeShapeVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(box_vertex), (void *)OffsetOf(box_vertex, P));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(box_vertex), (void *)OffsetOf(box_vertex, N));

    Renderer->InstanceVertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->InstanceVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(box_instance)*Renderer->MaxInstanceCount, 0, GL_STREAM_DRAW);

    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(box_instance), (void *)OffsetOf(box_instance, P));
    glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(box_instance), (void *)OffsetOf(box_instance, Dim));
    glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(box_instance), (void *)OffsetOf(box_instance, Orient));
    glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, true, sizeof(box_instance), (void *)OffsetOf(box_instance, Color));

    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);

    
    string VertexSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
layout (location=0) in vec3 VertexPosition;
layout (location=1) in vec3 VertexNormal;

layout (location=2) in vec3 InstancePosition;
layout (location=3) in vec3 InstanceDim;
layout (location=4) in vec4 InstanceOrient;
layout (location=5) in vec4 InstanceColor;

uniform mat4 Transform;
uniform vec3 MouseWorldP;

out vec4 VertColor;

vec3 Position;
vec3 Normal;

vec3 RotateByQuaternion(vec3 V, vec4 Q)
{
    // https://fgiesen.wordpress.com/2019/02/09/rotating-a-single-vector-using-a-quaternion/
    vec3 t = 2.0f * cross(Q.xyz, V);
    V += Q.w*t + cross(Q.xyz, t);
    return V;
}

float LightFrom(vec3 LightPosition, float Brightness)
{
    vec3 ToLight = LightPosition - Position;
    float LightStrength = Brightness / dot(ToLight, ToLight);
    float Factor = max(0.0, dot(Normal, normalize(ToLight)));
    float Result = LightStrength * Factor*Factor;
    return Result;
}

void main()
{
    Position = RotateByQuaternion(VertexPosition*InstanceDim*0.5, InstanceOrient) + InstancePosition;
    Normal = RotateByQuaternion(VertexNormal, InstanceOrient);
    vec4 Color = InstanceColor;

    VertColor.a = Color.a;

    float SkyLight = LightFrom(vec3(0, 100.0, 300.0), 30000.0);
    float MouseLight = LightFrom(MouseWorldP, 40.0);

    VertColor.rgb = Color.rgb*min(1.0, SkyLight + MouseLight);

    gl_Position = Transform*vec4(Position, 1.0);
}
)HereDoc"));


    string FragmentSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
in vec4 VertColor;
out vec4 FragColor;

void main()
{
    FragColor = VertColor;
    FragColor.rgb *= FragColor.a; // TODO(robin): Can we fix the blend mode so we don't need to do this?
    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
}
)HereDoc"));

    Renderer->Program = JS_GL_CreateCompileAndLinkProgram(VertexSource, FragmentSource);
    Renderer->Transform = glGetUniformLocation(Renderer->Program, S("Transform"));
    Renderer->MouseWorldP = glGetUniformLocation(Renderer->Program, S("MouseWorldP"));
}


function void Flush(renderer_boxes *Renderer)
{
    glBindVertexArray(Renderer->VertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, Renderer->InstanceVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, Renderer->InstanceCount*sizeof(box_instance), Renderer->InstanceData);

    glUseProgram(Renderer->Program);

    //glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 1);
    glDrawElementsInstanced(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 0, Renderer->InstanceCount);

    Renderer->InstanceCount = 0;
}

function void PushBox(renderer_boxes *Renderer, v3 Pos, v3 Dim, quaternion Orient = {0,0,0,1}, u32 Color = Color32_White)
{
    if(State.Boxes.InstanceCount >= Renderer->MaxInstanceCount)
    {
        Flush(Renderer);
    }

    box_instance *Box = Renderer->InstanceData + Renderer->InstanceCount++;
    Box->P = Pos;
    Box->Dim = Dim;
    Box->Orient = Orient;
    Box->Color = Color;
}
