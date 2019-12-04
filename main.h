

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


struct data_box
{
    v3 P;
    quaternion Orient;
    f32 tSmooth;
    f32 tFlyToMouse;

    string Name;
    string Email;
};

struct data_box_array
{
    string FavoriteFruit;
    u32 Count;
    u32 Capacity;
    u32 Selected;
    data_box *Data;
};

struct state
{
    u32 BoxArrayCount;
    data_box_array BoxArrays[8];
    u32 SelectedBoxArray;

    orbit_camera Camera;
    v2 LastMousePos;

    user_input Input;
    renderer Renderer;

    // NOTE(robin): Grows dynamically, should not be cleared
    memory_arena PermanentMemory;

    // NOTE(robin): Cleared every frame
    memory_arena FrameMemory;
};

