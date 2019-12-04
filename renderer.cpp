
function void AllocateBuffers(memory_arena *Memory, renderer_text *Renderer, u32 MaxIndexCount, u32 MaxVertexCount = (1 << 16) - 1)
{
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
function void InitTextRendering(memory_arena *PermMem, memory_arena *TmpMem, renderer_text *Renderer)
{
    u32 MaxVertexCount = (1 << 16) - 1;
    u32 MaxIndexCount = (MaxVertexCount*6) / 4;

    Renderer->MaxIndexCount = MaxIndexCount;
    Renderer->MaxVertexCount = MaxVertexCount;

    Renderer->Vertices = PushArray(PermMem, vertex, MaxVertexCount, ArenaFlag_NoClear);
    Renderer->Indices = PushArray(PermMem, u16, MaxIndexCount, ArenaFlag_NoClear);

    Renderer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Renderer->VertexArray);

    Renderer->VertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*Renderer->MaxVertexCount*sizeof(vertex), 0, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, P));
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(vertex), (void *)OffsetOf(vertex, UV));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (void *)OffsetOf(vertex, C));


    Renderer->IndexBuffer = glCreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Renderer->MaxIndexCount*sizeof(u16), 0, GL_STREAM_DRAW);

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
        font_atlas_char *Char = Renderer->Geometry + i;

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

    Renderer->Texture = glCreateTexture();
    glBindTexture(GL_TEXTURE_2D, Renderer->Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AtlasWidth, AtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, Pixels, PixelsSize/4, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);



    string VertexSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
uniform mat4 Transform;

layout (location=0) in vec3 Position;
layout (location=1) in vec2 UV;
layout (location=2) in vec4 Color;

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
}
)HereDoc"));

    Renderer->Program = JS_GL_CreateCompileAndLinkProgram(VertexSource, FragmentSource);
    Renderer->Transform = glGetUniformLocation(Renderer->Program, S("Transform"));
    Renderer->TextureSampler = glGetUniformLocation(Renderer->Program, S("TextureSampler"));

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(Renderer->Program);
    glUniform1i(Renderer->TextureSampler, 0);

}

function void
Flush(renderer_text *Renderer)
{
    glBindVertexArray(Renderer->VertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, Renderer->VertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, Renderer->VertexCount*sizeof(vertex), Renderer->Vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, Renderer->IndexCount*sizeof(u16), Renderer->Indices);

    glBindTexture(GL_TEXTURE_2D, Renderer->Texture);

    glUseProgram(Renderer->Program);

    glDrawElements(GL_TRIANGLES, Renderer->IndexCount, GL_UNSIGNED_SHORT, 0);

    Renderer->TrianglesRendered += Renderer->IndexCount/3;
    Renderer->IndexCount = 0;
    Renderer->VertexCount = 0;
}


function target_vertices_indices
AllocateVerticesAndIndices(renderer_text *Renderer, u32 RequestedVertexCount, u32 RequestedIndexCount)
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

function void
PushText(renderer_text *Renderer, string Text, m3x4 const &Transform = IdentityMatrix3x4, u32 Color = Color32_White)
{
    v2 TextAdvance = {};
    font_atlas_char *Geometry = Renderer->Geometry;
    target_vertices_indices Target = AllocateVerticesAndIndices(Renderer, Text.Size*4, Text.Size*6);

    for(size i = 0; i < Text.Size; ++i)
    {
        char Char = Text.Contents[i];
        if(Char == '\n')
        {
            TextAdvance.x = 0;
            TextAdvance.y -= FONT_GLYPH_SIZE / 256.0f;
            Renderer->VertexCount -= 4;
            Renderer->IndexCount -= 6;
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

        Target.I[0] = Target.FirstIndex + 2;
        Target.I[1] = Target.FirstIndex + 1;
        Target.I[2] = Target.FirstIndex + 0;

        Target.I[3] = Target.FirstIndex + 1;
        Target.I[4] = Target.FirstIndex + 2;
        Target.I[5] = Target.FirstIndex + 3;

        Target.I += 6;
        Target.V += 4;
        Target.FirstIndex += 4;

    }
}

function void
InitBoxRendering(memory_arena *PermMem, memory_arena *TmpMem, renderer_boxes *Renderer, u32 MaxInstanceCount = (1 << 16))
{
    Renderer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Renderer->VertexArray);

    {


//         .4------5     4------5     4------5     4------5     4------5.
//       .' |    .'|    /|     /|     |      |     |\     |\    |`.    | `.
//      0---+--1'  |   0-+----1 |     0------1     | 0----+-1   |  `0--+---1
//      |   |  |   |   | |    | |     |      |     | |    | |   |   |  |   |
//      |  ,6--+---7   | 6----+-7     6------7     6-+----7 |   6---+--7   |
//      |.'    | .'    |/     |/      |      |      \|     \|    `. |   `. |
//      2------3'      2------3       2------3       2------3      `2------3

        v3 P0 = {-0.5f, -0.5f,  0.5f};
        v3 P1 = { 0.5f, -0.5f,  0.5f};
        v3 P2 = {-0.5f, -0.5f, -0.5f};
        v3 P3 = { 0.5f, -0.5f, -0.5f};
        v3 P4 = {-0.5f,  0.5f,  0.5f};
        v3 P5 = { 0.5f,  0.5f,  0.5f};
        v3 P6 = {-0.5f,  0.5f, -0.5f};
        v3 P7 = { 0.5f,  0.5f, -0.5f};

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

        Renderer->CubeShapeVertexBuffer = glCreateBuffer();
        glBindBuffer(GL_ARRAY_BUFFER, Renderer->CubeShapeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(box_vertex), (void *)OffsetOf(box_vertex, P));
        glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(box_vertex), (void *)OffsetOf(box_vertex, N));
    }



    {
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

        Renderer->CubeShapeIndexBuffer = glCreateBuffer();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->CubeShapeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
    }



    {
        size InstanceBufferSize = sizeof(box_instance)*MaxInstanceCount;
        Renderer->MaxInstanceCount = MaxInstanceCount;
        Renderer->InstanceData = (box_instance *)PushSize(PermMem, InstanceBufferSize, ArenaFlag_NoClear);

        Renderer->InstanceVertexBuffer = glCreateBuffer();
        glBindBuffer(GL_ARRAY_BUFFER, Renderer->InstanceVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, InstanceBufferSize, 0, GL_STREAM_DRAW);

        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
        glEnableVertexAttribArray(5);

        glVertexAttribDivisor(2, 1);
        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);

        glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(box_instance), (void *)OffsetOf(box_instance, P));
        glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(box_instance), (void *)OffsetOf(box_instance, Dim));
        glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(box_instance), (void *)OffsetOf(box_instance, Orient));
        glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, true, sizeof(box_instance), (void *)OffsetOf(box_instance, Color));
    }

    
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
    Position = RotateByQuaternion(VertexPosition*InstanceDim, InstanceOrient) + InstancePosition;
    Normal = RotateByQuaternion(VertexNormal, InstanceOrient);
    vec4 Color = InstanceColor;

    float SkyLight = LightFrom(vec3(0,-100.0, 300.0), 30000.0);
    float MouseLight = LightFrom(MouseWorldP, 40.0);

    VertColor.a = Color.a;
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
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0, Renderer->InstanceCount);

    Renderer->TrianglesRendered += 36*Renderer->InstanceCount;
    Renderer->InstanceCount = 0;
}

function void PushBox(renderer_boxes *Renderer, v3 Pos, v3 Dim, quaternion Orient = {0,0,0,1}, u32 Color = Color32_White)
{
    if(Renderer->InstanceCount >= Renderer->MaxInstanceCount)
    {
        Flush(Renderer);
    }

    box_instance *Box = Renderer->InstanceData + Renderer->InstanceCount++;
    Box->P = Pos;
    Box->Dim = Dim;
    Box->Orient = Orient;
    Box->Color = Color;
}


function void
InitPostProcessing(memory_arena *PermMem, memory_arena *TmpMem, renderer_postprocessing *Renderer)
{
    v2 Rectangle[] = 
    {
        {-1, -1},
        { 1, -1},
        {-1,  1},

        { 1, -1},
        { 1,  1},
        {-1,  1},
    };

    Renderer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Renderer->VertexArray);

    Renderer->RectangleBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->RectangleBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Rectangle), Rectangle, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(v2), 0);
    
    string VertexSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
layout (location=0) in vec2 Position;

out vec4 VertColor;
out vec2 UV;

void main()
{
    UV = 0.5*Position + vec2(0.5);
    gl_Position = vec4(Position, 0.0, 1.0);
}
)HereDoc"));


    string FragmentSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
in vec2 UV;
out vec4 FragColor;

uniform sampler2D FramebufferSampler;

void main()
{
    FragColor = texture(FramebufferSampler, UV);
    FragColor.rgb = LinearToSRGB_Approx(FragColor.rgb);
}
)HereDoc"));

    Renderer->Program = JS_GL_CreateCompileAndLinkProgram(VertexSource, FragmentSource);
    Renderer->FramebufferSampler = glGetUniformLocation(Renderer->Program, S("FramebufferSampler"));
}

function void ResizeRendering(renderer *Renderer, u32 Width, u32 Height)
{
    glBindTexture(GL_TEXTURE_2D, Renderer->FramebufferColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Width, Height, 0, GL_RGBA, GL_FLOAT, 0, 0, 0);
    glBindTexture(GL_TEXTURE_2D, Renderer->FramebufferDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, Width, Height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Width, Height);
    glBindFramebuffer(GL_FRAMEBUFFER, Renderer->Framebuffer);
    glViewport(0, 0, Width, Height);
}

import_from_js size JS_GetBusMeshSize();
import_from_js void JS_GetBusMesh(void *Ptr);
function void InitMeshRendering(memory_arena *PermMem, memory_arena *TmpMem, renderer_mesh *Renderer)
{
    buffer Data = PushBuffer(TmpMem, JS_GetBusMeshSize(), ArenaFlag_NoClear);
    JS_GetBusMesh(Data.Contents);

    Renderer->VertexCount = *ReadType(&Data, u32);
    mesh_vertex *Verts = (mesh_vertex *)Read(&Data, sizeof(mesh_vertex)*Renderer->VertexCount);

    Assert((Data.Size % sizeof(u16)) == 0);
    Renderer->IndexCount = Data.Size/sizeof(u16);

    u16 *Indices = (u16 *)Read(&Data, Data.Size);

    Assert(Verts);
    Assert(Indices);

    Renderer->VertexArray = glCreateVertexArray();
    glBindVertexArray(Renderer->VertexArray);

    Renderer->IndexBuffer = glCreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Renderer->IndexCount*sizeof(u16), Indices, GL_STATIC_DRAW);

    Renderer->VertexBuffer = glCreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, Renderer->VertexCount*sizeof(mesh_vertex), Verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(mesh_vertex), (void *)OffsetOf(mesh_vertex, P));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(mesh_vertex), (void *)OffsetOf(mesh_vertex, N));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(mesh_vertex), (void *)OffsetOf(mesh_vertex, C));

    string VertexSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
layout (location=0) in vec3 Position;
layout (location=1) in vec3 VNormal;
layout (location=2) in vec4 Color;

uniform mat4 ViewTransform;
uniform mat4 ObjectTransform; // TODO(robin): Instancing
uniform vec3 MouseWorldP;

out vec4 VertColor;

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
    //Position = RotateByQuaternion(VertexPosition*InstanceDim, InstanceOrient) + InstancePosition;
    //Normal = RotateByQuaternion(VertexNormal, InstanceOrient);
    Normal = normalize(mat3(ObjectTransform) * VNormal);

    float SkyLight = LightFrom(vec3(0,-100.0, 300.0), 300000.0);

    VertColor.a = Color.a;
    VertColor.rgb = Color.rgb*min(1.0, SkyLight);

    gl_Position = (ViewTransform*ObjectTransform) * vec4(Position, 1.0);
}
)HereDoc"));


    string FragmentSource = Concat(TmpMem, CommonShaderHeader, S(R"HereDoc(
in vec4 VertColor;
out vec4 FragColor;

void main()
{
    FragColor = VertColor;
}
)HereDoc"));

    Renderer->Program = JS_GL_CreateCompileAndLinkProgram(VertexSource, FragmentSource);
    Renderer->ViewTransform = glGetUniformLocation(Renderer->Program, S("ViewTransform"));
    Renderer->ObjectTransform = glGetUniformLocation(Renderer->Program, S("ObjectTransform"));
    Renderer->MouseWorldP = glGetUniformLocation(Renderer->Program, S("MouseWorldP"));
}

function void InitRenderer(memory_arena *PermMem, memory_arena *TmpMem, renderer *Renderer)
{
    InitBoxRendering(PermMem, TmpMem, &Renderer->Boxes);
    InitTextRendering(PermMem, TmpMem, &Renderer->Text);
    InitMeshRendering(PermMem, TmpMem, &Renderer->Mesh);
    InitPostProcessing(PermMem, TmpMem, &Renderer->PostProcessing);

    Renderer->FramebufferColorTexture = glCreateTexture();
    glBindTexture(GL_TEXTURE_2D, Renderer->FramebufferColorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    Renderer->FramebufferDepthTexture = glCreateTexture();
    glBindTexture(GL_TEXTURE_2D, Renderer->FramebufferDepthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    Renderer->Framebuffer = glCreateFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, Renderer->Framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Renderer->FramebufferColorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Renderer->FramebufferDepthTexture, 0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

