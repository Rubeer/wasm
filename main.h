
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
    u32 C;
};

struct opengl
{
    GLuint VertexBuffer;
    GLuint IndexBuffer;

    GLuint VertexArray;

    GLuint Program;
    // Uniforms
    GLuint Projection;
};

struct state
{
    opengl OpenGL;

    u32 VertexCount;
    u32 IndexCount;
    vertex Vertices[1 << 16];
    u16 Indices[ArrayCount(Vertices)*6*6];

    v3 LastMouseWorldP;
};
