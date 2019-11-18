

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


#define BOX_COUNT 2048
struct box_animation
{
    quaternion Orient;
    f32 tSmooth;
    f32 tFlyToMouse;
};

struct state
{
    box_animation *BoxAnimations;
    u32 SelectedBoxIndex;

    user_input Input;
    renderer_default Default;
    renderer_text Text;

    // NOTE(robin): Grows dynamically, should not be cleared
    memory_arena PermanentMemory;

    // NOTE(robin): Cleared every frame
    memory_arena FrameMemory;
};

