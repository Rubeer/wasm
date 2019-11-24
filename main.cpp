
extern "C" unsigned char __heap_base;
extern "C" unsigned char __tls_size;
extern "C" unsigned char __global_base;
extern "C" unsigned char __data_end;


#include "util.h"
#include "math.h"
#include "webgl2_defs.h"

#include "renderer.h"
#include "main.h"
global state State;

#include "util.cpp"
#include "renderer.cpp"



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

export_to_js void MouseLeave()
{
    if(State.Input.MouseLeft.EndedDown)
    {
        MouseLeft(false);
    }
}

export_to_js void KeyPress(u32 KeyCode, bool EndedDown)
{
    if(KeyCode < ArrayCount(State.Input.Keys))
    {
        AddButtonTransition(&State.Input.Keys[KeyCode], EndedDown);
    }
}

export_to_js void MouseWheel(f32 DeltaY)
{
    State.Camera.Dolly += 0.002f*State.Camera.Dolly*DeltaY;
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


export_to_js void Init()
{
    memory_arena *PermanentMemory = &State.PermanentMemory;
    memory_arena *FrameMemory = &State.FrameMemory;
    PermanentMemory->Buffer.Size = GetHeapSize();
    PermanentMemory->Buffer.Contents = (char *)&__heap_base;
    FrameMemory->Buffer = PushBuffer(PermanentMemory, Megabytes(8), ArenaFlag_NoClear);

    //InitDefaultRendering(PermanentMemory, FrameMemory, &State.Default);
    InitTextRendering(PermanentMemory, FrameMemory, &State.Text);
    InitBoxRendering(PermanentMemory, FrameMemory, &State.Boxes);
    InitPostProcessing(PermanentMemory, FrameMemory, &State.PostProcessing);

    glEnable(GL_DEPTH_TEST);

    State.FramebufferColorTexture = glCreateTexture();
    glBindTexture(GL_TEXTURE_2D, State.FramebufferColorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    State.FramebufferDepthTexture = glCreateTexture();
    glBindTexture(GL_TEXTURE_2D, State.FramebufferDepthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    State.Framebuffer = glCreateFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, State.Framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, State.FramebufferColorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, State.FramebufferDepthTexture, 0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    State.MaxBoxCount = 40960;
    State.BoxCount = State.MaxBoxCount;
    State.BoxAnimations = PushArray(PermanentMemory, box_animation, State.MaxBoxCount);
    for(u32 i = 0; i < State.MaxBoxCount; ++i)
    {
        State.BoxAnimations[i].Orient.w = 1;
    }
    State.SelectedBoxIndex = U32Max;
    
    State.Camera.Tilt = 0.25f;
    f32 Radius = 150.0f;//(f32)State.BoxCount * 0.02f;
    State.Camera.Dolly = Radius + 18;

}

function void Resize(u32 Width, u32 Height)
{
    State.LastWidth = Width;
    State.LastHeight = Height;
 
    glBindTexture(GL_TEXTURE_2D, State.FramebufferColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Width, Height, 0, GL_RGBA, GL_FLOAT, 0, 0, 0);
    glBindTexture(GL_TEXTURE_2D, State.FramebufferDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, Width, Height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Width, Height);
    glBindFramebuffer(GL_FRAMEBUFFER, State.Framebuffer);
    glViewport(0, 0, Width, Height);
}

export_to_js void UpdateAndRender(u32 Width, u32 Height, f32 DeltaTime)
{
    if(Width != State.LastWidth || Height != State.LastHeight)
    {
        Resize(Width, Height);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, State.Framebuffer);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    orbit_camera *Camera = &State.Camera;


    v2 MousePixels = State.Input.MousePosPixels;
    v2 RenderDim = v2{(f32)Width, (f32)Height};
    v2 MouseClipSpace = 2.0f*(MousePixels - 0.5f*RenderDim) / RenderDim;
    MouseClipSpace.y = -MouseClipSpace.y;

    v2 MousePos = MousePixels/(f32)Height;
    v2 MouseDiff = MousePos - State.LastMousePos;
    State.LastMousePos = MousePos;


    //State.BoxCount = (State.BoxCount + 1) % State.MaxBoxCount;
    //State.BoxCount = 32;
    f32 Radius = 150.0f;//(f32)State.BoxCount * 0.02f;

    local_persist f32 Anim[5] = {0, 0.15f, 0.415f, 0.865f, 0};
    f32 Speeds[ArrayCount(Anim)] = {0.000015f*Radius, 0.1f, 0.1f, 0.1f, 0.01f};
    f32 Curve[ArrayCount(Anim)];
    for(size i = 0; i < ArrayCount(Anim); ++i)
    {
        if(Anim[i] >= 1.0f)
        {
            Anim[i] -= 1.0f;
        }
        Curve[i] = SmoothCurve010(Anim[i]);
        Anim[i] += DeltaTime*Speeds[i];
    }

    v3 OrbitCenter = {0, 0, 0};
    constexpr v3 Up = {0, 0, 1};

    button *MouseLeft = &State.Input.MouseLeft;
    if(MouseLeft->EndedDown)
    {
        Camera->Orbit -= 0.1f*MouseDiff.x;
        Camera->Tilt -= 0.1f*MouseDiff.y;
    }


    m4x4_inv CameraTransform = MakeCameraOrbitTransform(Camera);
    //v3 CameraP = {0, -Radius - 18, 7};
    //v3 CameraZ = Normalize(CameraP - OrbitCenter);
    ////v3 CameraZ = Normalize(v3{0.2f,-1,0.2f});
    //v3 CameraX = Normalize(Cross(Up, CameraZ));
    //v3 CameraY = Cross(CameraZ, CameraX);

    f32 NearClip = 0.1f;
    f32 FarClip = Radius*2.5f + Camera->Dolly;
    f32 FocalLength = 1.0f;
    m4x4_inv Projection = PerspectiveProjectionTransform(Width, Height, NearClip, FarClip, FocalLength);


    m4x4_inv RenderTransform;
    RenderTransform.Forward = Projection.Forward * CameraTransform.Forward;
    RenderTransform.Inverse = CameraTransform.Inverse * Projection.Inverse;

    v3 MouseWorldP = {};
    {
        v4 FromCamera = {0,0,0,1};
        FromCamera.xyz = Camera->P - 10.0f*Camera->P;
        v4 FromCameraClip = RenderTransform.Forward * FromCamera;

        v4 ClipPos;
        ClipPos.xy = MouseClipSpace * FromCameraClip.w;
        ClipPos.zw = FromCameraClip.zw;

        MouseWorldP = (RenderTransform.Inverse * ClipPos).xyz;
    }



    hit_test HitTest = {};
    HitTest.RayOrigin = Camera->P;
    HitTest.RayOriginOffset = MouseWorldP;
    HitTest.RayDir = Normalize(MouseWorldP - Camera->P);
    HitTest.LastHitDistance = F32Max;

    u32 ClosestIndex = U32Max;
    f32 ClosestDistance = F32Max;

    glUseProgram(State.Text.Common.Program);
    glUniformMatrix4fv(State.Text.Common.Transform, true, &RenderTransform.Forward);

#if 0
    glUseProgram(State.Default.Common.Program);
    glUniformMatrix4fv(State.Default.Common.Transform, true, &RenderTransform.Forward);
    glUniform3f(State.Default.MouseWorldP, HitTest.RayOrigin + HitTest.RayDir*7.5f);
#endif

    glUseProgram(State.Boxes.Program);
    glUniformMatrix4fv(State.Boxes.Transform, true, &RenderTransform.Forward);
    glUniform3f(State.Boxes.MouseWorldP, HitTest.RayOrigin + HitTest.RayDir*7.5f);

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

    v3 PrevP = {};

    random_state Random = DefaultSeed();
    for(u32 i = 0; i < State.BoxCount; ++i)
    {
        box_animation *Box = State.BoxAnimations + i;

        f32 V = (f32)i / (f32)State.BoxCount;

        v3 P = OrbitCenter;
        P.x += (25.0f + Radius*0.1f)*RandomBilateral(&Random) + Radius*CosineApproxN(V+Anim[0]);
        P.y += (25.0f + Radius*0.1f)*RandomBilateral(&Random) + Radius*SineApproxN(V+Anim[0]);
        P.z += CosineApproxN(V + RandomBilateral(&Random)) * 50.0f;

        v3 Dim = v3{0.9f, 0.9f, 0.9f};
        v3 HalfDim = Dim*0.5f;

        bool Selected = (i == State.SelectedBoxIndex);
        Box->tFlyToMouse += Selected ? DeltaTime : -DeltaTime;
        Box->tFlyToMouse = Clamp01(Box->tFlyToMouse);
        v3 FlyToP = HitTest.RayOrigin + HitTest.RayDir*2.5f;
        P = Lerp(P, SmoothCurve01(Box->tFlyToMouse), FlyToP);

        if(i != 0)
        {
            v3 P0toP1 = PrevP - P;
            f32 Len = Length(P0toP1);
            v3 Dir = P0toP1 / Len;
            //m3x4 LineTransform = Translation(P) * MatrixAsColumns3x4(X, Y, Z) * Translation(0,0,HalfLen) * Scaling(0.05f,0.05f,HalfLen);
            v3 LineP = P + P0toP1*0.5f;
            v3 LineDim = {0.1f, 0.1f, Len};
            PushBox(&State.Boxes, LineP, LineDim, QuaternionLookAt(Dir, Up));
            PrevP = P;
        }

        // TODO(robin) Symbolically solve
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
            v3 RotUp = {0,1,0};//v3 CamUp = {Camera->Y.y, Camera->Y.z, Camera->Y.x};
            quaternion RotationTarget = QuaternionLookAt(Normalize(P - Camera->P), RotUp);
            Box->Orient = LerpShortestPath(Box->Orient, Box->tSmooth*4.0f*DeltaTime, RotationTarget);
        }
        else
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

        // TODO(robin) Symbolically solve
        //m3x4 UnscaledBoxTransform = Translation(P) * QuaternionRotationMatrix(Box->Orient);//XYZRotationN(Angles);
        //m3x4 BoxTransform = UnscaledBoxTransform * Scaling(HalfDim);

        f32 B = (0.5f*(1.0f + SineApproxN(V)));
        f32 R = (0.5f*(1.0f + SineApproxN(V + 1.0f/3.0f)));
        f32 G = (0.5f*(1.0f + SineApproxN(V + 2.0f/3.0f)));
        //u32 C = RandomSolidColor(&Random);
        u32 C = Pack01RGBA255(R,G,B);
        //PushBox(&State.Default, P, Dim, Box->Orient, C,C,C,C,C,C);
        //PushBox(&State.Default, BoxTransform, C,C,C,C,C,C);
        PushBox(&State.Boxes, P, Dim, Box->Orient, C);


        f32 CameraDistance = Length(Camera->P - P);
#if 1
        if(CameraDistance < 15.0f)
        {
            f32 TextAlpha = 1.0f - SmoothStep(5.0f, CameraDistance, 15.0f);

            m3x4 UnscaledBoxTransform = Translation(P) * QuaternionRotationMatrix(Box->Orient);//XYZRotationN(Angles);
            m3x4 MakeUpright = MatrixAsRows3x4(v3{1,0,0}, v3{0,0,1}, v3{0,1,0});
            // TODO(robin) Symbolically solve
            m3x4 TextTransform = UnscaledBoxTransform * Translation(-HalfDim.x, -HalfDim.y*1.002f, HalfDim.z) * MakeUpright;
            char Buf[128];
            string Text = FormatText(Buf, "Box #%u\n"
                                          "x %f\n"
                                          "y %f\n"
                                          "z %f\n",
                                          i, P.x, P.y, P.z);

            u32 Color = Pack01RGBA255(1,1,1, TextAlpha);
            PushText(&State.Text, Text, TextTransform, Color);
        }
#endif
    }


    if(MouseLeft->HalfTransitionCount && MouseLeft->EndedDown)
    {
        MouseLeft->HalfTransitionCount = 0;
        State.SelectedBoxIndex = ClosestIndex;
    }

    //m3x4 PlanetTransform = Translation(OrbitCenter) * Scaling(Minimum(Radius*0.1f, 10.0f)) * ZRotationN(Anim[4]);// * Scaling(10.0f);
    //m3x4 InvObjectTransform = ZRotationN(-0.6f) * Scaling(1.0f/10, 1.0f/9, 1.0f/9) * Translation(-5, 0, 0);
    //u32 C = 0xA0808080;
    //PushBox(&State.Default, PlanetTransform, C,C,C,C,C,C);
    //PushBox(Translation(MouseWorldP)*Scaling(0.05f));


    Flush(&State.Boxes);
    //Flush(&State.Default.Common);
    Flush(&State.Text.Common);

    m4x4 HUDProj = HUDProjection(Width, Height);
    glUseProgram(State.Text.Common.Program);
    glUniformMatrix4fv(State.Text.Common.Transform, true, &HUDProj);

    char Buf[32];
    string HudText = FormatText(Buf, "%f fps (%f ms)", 1.0f / DeltaTime, DeltaTime*1000.0f);
    PushText(&State.Text, HudText, Scaling(0.4f));

    Flush(&State.Text.Common);

    Reset(&State.FrameMemory);

#if 0
    glBindFramebuffer(GL_READ_FRAMEBUFFER, State.Framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, (GLint)Width, (GLint)Height,
                      0, 0, (GLint)Width, (GLint)Height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(State.PostProcessing.Program);
    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D, State.FramebufferColorTexture);
    glUniform1i(State.PostProcessing.FramebufferSampler, 1);
    glBindVertexArray(State.PostProcessing.VertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glActiveTexture(GL_TEXTURE0);
#endif
}


