

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


struct box_animation
{
    quaternion Orient;
    f32 tSmooth;
    f32 tFlyToMouse;
};

struct state
{
    u32 BoxCount;
    u32 MaxBoxCount;
    box_animation *BoxAnimations;
    u32 SelectedBoxIndex;

    user_input Input;
    renderer_default Default;
    renderer_text Text;
    renderer_boxes Boxes;

    // NOTE(robin): Grows dynamically, should not be cleared
    memory_arena PermanentMemory;

    // NOTE(robin): Cleared every frame
    memory_arena FrameMemory;
};

