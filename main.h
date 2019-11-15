

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


#define BOX_COUNT 4096
struct box_animation
{
    quaternion Orient;
    f32 tSmooth;
    f32 tFlyToMouse;
};

struct state
{
    box_animation BoxAnimations[BOX_COUNT];
    u32 SelectedBoxIndex;

    user_input Input;
    renderer_default Default;
    renderer_text Text;
};

