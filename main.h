
struct button
{
    u32 HalfTransitionCount;
    bool EndedDown;
};

struct user_input
{
    v2 MousePosPixels;
    button MouseLeft;
    button MouseRight;
    button Keys[256];
};

struct vertex
{
    v3 P;
    v2 UV;
    u32 C;
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

struct font_atlas
{
    GLuint Texture;
    GLuint Program;
    GLuint TextureSampler;
    GLuint Transform;
    font_atlas_char Geometry[256];
};

struct renderer
{
    GLuint VertexBuffer;
    GLuint IndexBuffer;

    GLuint VertexArray;
    GLuint FontTexture;

    GLuint Program;
    // Uniforms
    GLuint Transform;

    font_atlas FontAtlas;
};

struct state
{
    renderer Renderer;

    u32 VertexCount;
    u32 IndexCount;
    vertex Vertices[1 << 16];
    u16 Indices[ArrayCount(Vertices)*6*6];

    v3 LastMouseWorldP;
};
