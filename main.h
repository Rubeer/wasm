

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

    string Text;
    u32 Color;
};

struct state
{
    u32 DataBoxCount;
    u32 MaxDataBoxes;
    data_box *DataBoxes;
    u32 SelectedDataBox;

    orbit_camera Camera;
    v2 LastMousePos;

    user_input Input;
    renderer Renderer;

    // NOTE(robin): Grows dynamically, should not be cleared
    memory_arena PermanentMemory;

    // NOTE(robin): Cleared every frame
    memory_arena FrameMemory;
};

