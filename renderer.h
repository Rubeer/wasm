

enum : u32
{
    Color32_White  = 0xFFFFFFFF,
    Color32_Black  = 0xFF000000,
    Color32_Red    = 0xFF0000FF,
    Color32_Green  = 0xFF00FF00,
    Color32_Blue   = 0xFFFF0000,
};

struct vertex
{
    v3 P;
    v3 N;
    v2 UV;
    u32 C;
};

struct target_vertices_indices
{
    vertex *V;
    u16 *I;
    u16 FirstIndex;
};


#define FONT_GLYPH_SIZE 32
#pragma pack(push, 1)
struct font_atlas_char_packed
{
    u8 MinX;
    u8 MinY;
    u8 MaxX;
    u8 MaxY;

    s8 OffX;
    s8 OffY;
    s8 Advance;
};
#pragma pack(pop)

struct font_atlas_char
{
    v2 Min;
    v2 Max;
    v2 Offset;
    float Advance;
};

struct renderer_common
{
    GLuint Program;
    GLuint Transform;

    GLuint VertexBuffer;
    GLuint VertexArray;
    GLuint IndexBuffer;

    u32 VertexCount;
    u32 MaxVertexCount;
    u32 IndexCount;
    u32 MaxIndexCount;

    vertex *Vertices;
    u16 *Indices;
};


struct renderer_text
{
    renderer_common Common;

    GLuint Texture;
    GLuint TextureSampler;
    font_atlas_char Geometry[256];
};

struct renderer_default
{
    renderer_common Common;
    GLuint MouseWorldP;
};
