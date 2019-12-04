

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

struct box_vertex
{
    v3 P;
    v3 N;
};

struct box_instance
{
    v3 P;
    v3 Dim;
    quaternion Orient;
    u32 Color;
};

struct renderer_boxes
{
    GLuint Program;
    GLuint Transform;
    GLuint MouseWorldP;

    GLuint VertexArray;

    GLuint CubeShapeVertexBuffer;
    GLuint CubeShapeIndexBuffer;

    u32 MaxInstanceCount;
    u32 InstanceCount;
    GLuint InstanceVertexBuffer;
    box_instance *InstanceData;

    u32 TrianglesRendered;
};


struct renderer_text
{
    GLuint Program;
    GLuint Transform;

    GLuint VertexArray;

    GLuint VertexBuffer;
    GLuint IndexBuffer;

    u32 VertexCount;
    u32 MaxVertexCount;
    u32 IndexCount;
    u32 MaxIndexCount;

    vertex *Vertices;
    u16 *Indices;

    GLuint Texture;
    GLuint TextureSampler;
    font_atlas_char Geometry[256];

    u32 TrianglesRendered;
};

struct mesh_vertex
{
    v3 P;
    v3 N;
    u32 C;
};


struct renderer_mesh
{
    GLuint Program;
    GLuint ViewTransform;
    GLuint ObjectTransform; // TODO(robin): Instancing
    GLuint MouseWorldP;

    GLuint VertexArray;
    GLuint VertexBuffer;
    GLuint IndexBuffer;

    u32 VertexCount;
    u32 IndexCount;
};

struct renderer_postprocessing
{
    GLuint Program;
    GLuint FramebufferSampler;
};

struct renderer_skybox
{
    GLuint Program;
    GLuint SkyboxSampler;
    GLuint Transform;

    GLuint Texture;
};


struct renderer
{
    renderer_boxes Boxes;
    renderer_text Text;
    renderer_mesh Mesh;
    renderer_postprocessing PostProcessing;
    renderer_skybox Skybox;

    GLuint RectangleBuffer;
    GLuint RectangleVertexArray;

    GLuint FramebufferColorTexture;
    GLuint FramebufferDepthTexture;
    GLuint Framebuffer;
    u32 LastWidth;
    u32 LastHeight;
};
