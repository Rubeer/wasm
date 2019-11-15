

#include "util.h"
#include "math.h"
#include "webgl2_defs.h"

#include "renderer.h"
#include "main.h"

#include "renderer.cpp"

global state State;

global u8 __FrameMemory[1024*1024*8];
global memory_arena FrameMemory = MemoryArenaFromByteArray(__FrameMemory);


export_to_js void MouseMove(s32 X, s32 Y)
{
    State.Input.MousePosPixels = {(f32)X, (f32)Y};
}

function void AddButtonTransition(button *Button, bool EndedDown)
{
    Button->EndedDown = EndedDown;
    Button->HalfTransitionCount++;
}

export_to_js void MouseLeft(bool EndedDown)
{
    AddButtonTransition(&State.Input.MouseLeft, EndedDown);
}

export_to_js void KeyPress(u32 KeyCode, bool EndedDown)
{
    if(KeyCode < ArrayCount(State.Input.Keys))
    {
        AddButtonTransition(&State.Input.Keys[KeyCode], EndedDown);
    }
}



struct hit_test
{
    v3 RayOrigin;
    v3 RayOriginOffset;
    v3 RayDir;
    f32 LastHitDistance;
};

function bool RaycastBox(hit_test *HitTest, m3x4 const &InverseBoxTransform)
{
    v3 RayOrigin = InverseBoxTransform * HitTest->RayOrigin;
    v3 RayOriginOffset = InverseBoxTransform * HitTest->RayOriginOffset;
    v3 RayDir = Normalize(RayOriginOffset - RayOrigin);

    v3 BoxMin = v3{-1, -1, -1};
    v3 BoxMax = v3{ 1,  1,  1};

    v3 tBoxMin = (BoxMin - RayOrigin) / RayDir;
    v3 tBoxMax = (BoxMax - RayOrigin) / RayDir;

    v3 tMin3 = Minimum(tBoxMin, tBoxMax);
    v3 tMax3 = Maximum(tBoxMin, tBoxMax);

    f32 tMin = Maximum(tMin3.x, Maximum(tMin3.y, tMin3.z));
    f32 tMax = Minimum(tMax3.x, Minimum(tMax3.y, tMax3.z));

    bool Hit = (tMin > 0.0f) && (tMin < tMax);
    HitTest->LastHitDistance = tMin;

    return Hit;
}


extern "C" unsigned char __heap_base;



export_to_js void Init()
{
    Printf("%u", (u32)&__heap_base / 1024);

    InitDefaultRendering(&FrameMemory, &State.Default);
    InitTextRendering(&FrameMemory, &State.Text);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    for(u32 i = 0; i < BOX_COUNT; ++i)
    {
        State.BoxAnimations[i].Orient.w = 1;
    }
    State.SelectedBoxIndex = U32Max;

}

export_to_js void UpdateAndRender(u32 Width, u32 Height, f32 DeltaTime)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, Width, Height);

    v2 MousePixels = State.Input.MousePosPixels;
    v2 RenderDim = v2{(f32)Width, (f32)Height};
    v2 MouseClipSpace = 2.0f*(MousePixels - 0.5f*RenderDim) / RenderDim;
    MouseClipSpace.y = -MouseClipSpace.y;


    local_persist f32 Anim[5] = {0, 0.15f, 0.415f, 0.865f, 0};
    f32 Speeds[ArrayCount(Anim)] = {0.001f, 0.1f, 0.1f, 0.1f, 0.01f};
    f32 Curve[ArrayCount(Anim)];
    for(size i = 0; i < ArrayCount(Anim); ++i)
    {
        if(Anim[i] >= 1.0f)
        {
            Anim[i] = 0.0f;
        }
        Curve[i] = SmoothCurve010(Anim[i]);
        Anim[i] += DeltaTime*Speeds[i];
    }

    v3 OrbitCenter = {0, 107, -6};
    v3 CameraP = {0, 0, 0};
    v3 Up = {0, 0, 1};

    v3 CameraZ = Normalize(CameraP - OrbitCenter);
    //v3 CameraZ = Normalize(v3{0.2f,-1,0.2f});
    v3 CameraX = Normalize(Cross(Up, CameraZ));
    v3 CameraY = Cross(CameraZ, CameraX);
    m4x4_inv Camera = CameraTransform(CameraX, CameraY, CameraZ, CameraP);

    f32 NearClip = 0.1f;
    f32 FarClip = 1000.0f;
    f32 FocalLength = 1.0f;
    m4x4_inv Projection = PerspectiveProjectionTransform(Width, Height, NearClip, FarClip, FocalLength);

    m4x4_inv RenderTransform;
    RenderTransform.Forward = Projection.Forward * Camera.Forward;
    RenderTransform.Inverse = Camera.Inverse * Projection.Inverse;

    v3 MouseWorldP = {};
    {
        v4 FromCamera = {0,0,0,1};
        FromCamera.xyz = CameraP - 10.0f*CameraZ;
        v4 FromCameraClip = RenderTransform.Forward * FromCamera;

        v4 ClipPos;
        ClipPos.xy = MouseClipSpace * FromCameraClip.w;
        ClipPos.zw = FromCameraClip.zw;

        MouseWorldP = (RenderTransform.Inverse * ClipPos).xyz;
    }


    random_state Random = DefaultSeed();

    hit_test HitTest = {};
    HitTest.RayOrigin = CameraP;
    HitTest.RayOriginOffset = MouseWorldP;
    HitTest.RayDir = Normalize(MouseWorldP - CameraP);
    HitTest.LastHitDistance = F32Max;

    u32 ClosestIndex = U32Max;
    f32 ClosestDistance = F32Max;

    glUseProgram(State.Text.Common.Program);
    glUniformMatrix4fv(State.Text.Common.Transform, true, &RenderTransform.Forward);

    glUseProgram(State.Default.Common.Program);
    glUniformMatrix4fv(State.Default.Common.Transform, true, &RenderTransform.Forward);
    glUniform3f(State.Default.MouseWorldP, HitTest.RayOrigin + HitTest.RayDir*7.5f);

    local_persist f32 RotSpeed;
    button *Keys = State.Input.Keys;
    if(Keys['A'].EndedDown)
    {
        RotSpeed -= RotSpeed*DeltaTime*2 + DeltaTime*0.03f; 
    }
    if(Keys['D'].EndedDown)
    {
        RotSpeed += RotSpeed*DeltaTime*2 + DeltaTime*0.03f; 
    }
    RotSpeed *= 1.0f - 4*DeltaTime;

    Anim[0] += RotSpeed;

    for(u32 i = 0; i < BOX_COUNT; ++i)
    {
        box_animation *Box = State.BoxAnimations + i;

        f32 V = i * (1.0f / BOX_COUNT);

        v3 P = OrbitCenter;
        P.x += 7.0f*RandomBilateral(&Random) + 100.0f*CosineApproxN(V+Anim[0]);
        P.y += 7.0f*RandomBilateral(&Random) + 100.0f*SineApproxN(V+Anim[0]);
        P.z += CosineApproxN(V + RandomBilateral(&Random)) * 4.0f;

        v3 Dim = v3{0.9f, 0.9f, 0.9f};
        v3 HalfDim = Dim*0.5f;

        bool Selected = (i == State.SelectedBoxIndex);
        Box->tFlyToMouse += Selected ? DeltaTime : -DeltaTime;
        Box->tFlyToMouse = Clamp01(Box->tFlyToMouse);

        {
            v3 FlyToP = HitTest.RayOrigin + HitTest.RayDir*2.5f;
            f32 t = SmoothCurve01(Box->tFlyToMouse);
            P = Lerp(P, t, FlyToP);
        }

        m3x4 InverseBoxTransform = Scaling(1.0f/HalfDim) * QuaternionRotationMatrix(Conjugate(Box->Orient)) * Translation(-P);
        bool RayHit = RaycastBox(&HitTest, InverseBoxTransform);


        v3 AngleDiff = {RandomBilateral(&Random),
                        RandomBilateral(&Random),
                        RandomBilateral(&Random)};

        if(!Selected)
        {
            // NOTE(robin): Idle rotation
            AngleDiff = AngleDiff * 0.1f * DeltaTime * (1.0f - Box->tSmooth);
            quaternion Diff = QuaternionFromAnglesN(AngleDiff);
            Box->Orient = Normalize(Box->Orient * Diff);
        }


        if(RayHit || Selected)
        {
            // NOTE(robin): Rotate to face forwards on mouse-over
            Box->tSmooth += DeltaTime*2.0f;
            Box->Orient = LerpShortestPath(Box->Orient, Box->tSmooth*4.0f*DeltaTime, quaternion{0,0,0,1});
        }
        if(!RayHit)
        {
            Box->tSmooth -= DeltaTime*2.0f;
        }
        Box->tSmooth = Clamp01(Box->tSmooth);

        if(RayHit && !Selected)
        {
            if(HitTest.LastHitDistance < ClosestDistance)
            {
                ClosestDistance = HitTest.LastHitDistance;
                ClosestIndex = i;
            }
        }

        m3x4 UnscaledBoxTransform = Translation(P) * QuaternionRotationMatrix(Box->Orient);//XYZRotationN(Angles);
        m3x4 BoxTransform = UnscaledBoxTransform * Scaling(HalfDim);

        f32 OneThirdCurve = Tau32 / 3.0f;
        f32 R = (0.5f*(1.0f + SineApproxN(V)));
        f32 G = (0.5f*(1.0f + SineApproxN(V + OneThirdCurve*2)));
        f32 B = (0.5f*(1.0f + SineApproxN(V + OneThirdCurve)));
        //u32 C = RandomSolidColor(&Random);
        u32 C = Pack01RGBA255(R,G,B);
        PushBox(&State.Default, BoxTransform, C,C,C,C,C,C);


        if(LengthSquared(CameraP - P) < Square(30.0f))
        {
            m3x4 MakeUpright = MatrixAsRows3x4(v3{1,0,0}, v3{0,0,1}, v3{0,1,0});
            m3x4 TextTransform = UnscaledBoxTransform * Translation(-HalfDim.x, -HalfDim.y*1.002f, HalfDim.z) * MakeUpright;
            char Buf[128];
            string Text = FormatText(Buf, 
                                          "Box #%u\n"
                                          "Position:\n"
                                          "x %f\n"
                                          "y %f\n"
                                          "z %f\n",
                                          i, P.x, P.y, P.z);

            PushText(&State.Text, Text, TextTransform);
        }
    }

    button *Left = &State.Input.MouseLeft;
    if(Left->HalfTransitionCount && Left->EndedDown)
    {
        Left->HalfTransitionCount = 0;
        State.SelectedBoxIndex = ClosestIndex;
    }

    m3x4 PlanetTransform = Translation(OrbitCenter) * Scaling(10.0f, 9, 9) * ZRotationN(Anim[4]);// * Scaling(10.0f);

    //m3x4 InvObjectTransform = ZRotationN(-0.6f) * Scaling(1.0f/10, 1.0f/9, 1.0f/9) * Translation(-5, 0, 0);
    u32 C = 0xA0808080;
    PushBox(&State.Default, PlanetTransform, C,C,C,C,C,C);

    //PushBox(Translation(MouseWorldP)*Scaling(0.05f));


    Flush(&State.Default.Common);
    Flush(&State.Text.Common);


    m4x4 HUDProj = HUDProjection(Width, Height);
    glUseProgram(State.Text.Common.Program);
    glUniformMatrix4fv(State.Text.Common.Transform, true, &HUDProj);


    glClear(GL_DEPTH_BUFFER_BIT);

    char Buf[32];
    string HudText = FormatText(Buf, "%f ms", DeltaTime*1000.0f);
    PushText(&State.Text, HudText, Scaling(0.3f));


    Flush(&State.Text.Common);

    Reset(&FrameMemory);
}


