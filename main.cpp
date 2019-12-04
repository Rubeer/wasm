
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

export_to_js void *AllocatePermanent(u32 Length)
{
    return PushSize(&State.PermanentMemory, Length);
}

export_to_js void UpdateDataBox(u32 Index, u32 TextLength, char *TextPtr, u32 Color)
{
    Assert(Index < State.DataBoxCount);
    data_box *Box = State.DataBoxes + Index;
    Box->Text = {TextLength, TextPtr};
    Box->Color = Color;
}

export_to_js u32 AddDataBox(u32 TextLength, char *TextPtr, u32 Color)
{
    Assert(State.DataBoxCount < State.MaxDataBoxes);

    u32 Index = State.DataBoxCount++;
    data_box *Box = State.DataBoxes + Index;
    Box->Orient = {0, 0, 0, 1};
    Box->tSmooth = 0;
    Box->tFlyToMouse = 0;

    UpdateDataBox(Index, TextLength, TextPtr, Color);

    return Index;
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

    InitRenderer(PermanentMemory, FrameMemory, &State.Renderer);

    State.SelectedDataBox = U32Max;
    State.MaxDataBoxes = (1 << 16);
    State.DataBoxes = PushArray(&State.PermanentMemory, data_box, State.MaxDataBoxes);
    
    State.Camera.Tilt = 0.15f;
    State.Camera.Dolly = 100;

}

export_to_js void UpdateAndRender(u32 Width, u32 Height, f32 DeltaTime)
{
    orbit_camera *Camera = &State.Camera;
    renderer *Renderer = &State.Renderer;

    {
        // NOTE(robin): Get averaged frame rate
        // TODO(robin): Sliding window might be better
        local_persist f32 dtSamples[4];
        local_persist u32 dtSampleIndex;
        dtSamples[dtSampleIndex] = DeltaTime;
        dtSampleIndex = (dtSampleIndex + 1) % ArrayCount(dtSamples);
        DeltaTime = 0;
        for(u32 i = 0; i < ArrayCount(dtSamples); ++i)
        {
            DeltaTime += dtSamples[i];
        }
        DeltaTime /= ArrayCount(dtSamples);

        if(DeltaTime < 0.001f || DeltaTime > 0.1f)
        {
            DeltaTime = 1.0f/60.0f;
        }
    }

    u32 LastFrameTriangleCount = Renderer->Boxes.TrianglesRendered +
                                 Renderer->Text.TrianglesRendered +
                                 Renderer->Mesh.IndexCount/3;

    Renderer->Boxes.TrianglesRendered = 0;
    Renderer->Text.TrianglesRendered = 0;

    if(Width != Renderer->LastWidth || Height != Renderer->LastHeight)
    {
        Renderer->LastWidth = Width;
        Renderer->LastHeight = Height;
        ResizeRendering(Renderer, Width, Height);
    }


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
        Camera->Orbit -= 0.2f*MouseDiff.x;
        Camera->Tilt -= 0.2f*MouseDiff.y;
    }


    m3x4 CameraRotation = ZRotationN(Camera->Orbit) * XRotationN(Camera->Tilt);
    Camera->P = CameraRotation * v3{0, 0, Camera->Dolly};
    Camera->X = GetColumn(CameraRotation, 0);
    Camera->Y = GetColumn(CameraRotation, 1);
    Camera->Z = GetColumn(CameraRotation, 2);
    m4x4_inv CameraTransform = MakeCameraTransform(Camera->X, Camera->Y, Camera->Z, Camera->P);
    quaternion LookAtCamera = MatrixOrientToQuaternion(CameraRotation * XRotationN(-0.25f));
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

    hit_test HitTest = {};
    HitTest.RayOrigin = Camera->P;
    HitTest.RayOriginOffset = MouseWorldP;
    HitTest.RayDir = Normalize(MouseWorldP - Camera->P);
    HitTest.LastHitDistance = F32Max;

    u32 ClosestIndex = U32Max;
    f32 ClosestDistance = F32Max;

    glUseProgram(Renderer->Text.Program);
    glUniformMatrix4fv(Renderer->Text.Transform, true, &RenderTransform.Forward);

    glUseProgram(Renderer->Boxes.Program);
    glUniformMatrix4fv(Renderer->Boxes.Transform, true, &RenderTransform.Forward);
    glUniform3f(Renderer->Boxes.MouseWorldP, HitTest.RayOrigin + HitTest.RayDir*7.5f);

    glBindFramebuffer(GL_FRAMEBUFFER, Renderer->Framebuffer);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // NOTE(robin): Draw skybox cubemap
    {
        glUseProgram(Renderer->Skybox.Program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, Renderer->Skybox.Texture);
        glUniform1i(Renderer->Skybox.SkyboxSampler, 0);
        m4x4 Transform = RenderTransform.Forward * To4x4(Translation(Camera->P));
        glUniformMatrix4fv(Renderer->Skybox.Transform, true, &Transform);
        glBindVertexArray(Renderer->Boxes.VertexArray);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
    }



    random_state Random = DefaultSeed();
    v3 PrevP = {};

    for(u32 BoxIndex = 0;
            BoxIndex < State.DataBoxCount;
            ++BoxIndex)
    {
        data_box *Box = State.DataBoxes  + BoxIndex;

        f32 V = (f32)BoxIndex / (f32)State.DataBoxCount;

        Box->P = OrbitCenter;
        Box->P.x += (0.5f + 0.5f*RandomUnilateral(&Random)) * 100*CosineApproxN(V+Anim[0]);
        Box->P.y += (0.5f + 0.5f*RandomUnilateral(&Random)) * 100*SineApproxN(V+Anim[0]);
        //Box->P.y += (15.0f)*RandomBilateral(&Random) + 60*SineApproxN(V+Anim[0]);
        Box->P.z += CosineApproxN(Anim[0] + V + RandomBilateral(&Random)) * 30.0f;

        // TODO(robin): Maybe reorder the code and roll this into PushText
        v2 TextDim = GetTextDim(&State.Renderer.Text, Box->Text);
        v2 Padding = {0.07f, 0.05f};
        TextDim += 2.0f*Padding;

        v3 Dim;
        Dim.x = Maximum(TextDim.x, 1.0f);
        Dim.z = Maximum(TextDim.y, 1.0f);
        Dim.y = Minimum(Dim.x, Dim.z);

        v3 HalfDim = Dim*0.5f;

        bool Selected = (BoxIndex == State.SelectedDataBox);
        Box->tFlyToMouse += Selected ? DeltaTime : -DeltaTime;
        Box->tFlyToMouse = Clamp01(Box->tFlyToMouse);
        v3 FlyToP = HitTest.RayOrigin + HitTest.RayDir*2.5f;
        Box->P = Lerp(Box->P, SmoothCurve01(Box->tFlyToMouse), FlyToP);

        if(BoxIndex)
        {
            v3 P0toP1 = PrevP - Box->P;
            f32 Len = Length(P0toP1);
            v3 Dir = P0toP1 / Len;
            //m3x4 LineTransform = Translation(P) * MatrixAsColumns3x4(X, Y, Z) * Translation(0,0,HalfLen) * Scaling(0.05f,0.05f,HalfLen);
            v3 LineP = Box->P + P0toP1*0.5f;
            v3 LineDim = {0.1f, 0.1f, Len};
            PushBox(&Renderer->Boxes, LineP, LineDim, QuaternionLookAt(Dir, Up));
        }
        PrevP = Box->P;

        // TODO(robin) Symbolically solve
        m3x4 InverseBoxTransform = Scaling(1.0f/HalfDim) * QuaternionRotationMatrix(Conjugate(Box->Orient)) * Translation(-Box->P);
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
            // NOTE(robin): Rotate to face camera on mouse-over
            Box->tSmooth += DeltaTime*2.0f;
            //v3 Dir = Normalize(Box->P - Camera->P);
            quaternion RotationTarget = LookAtCamera;//)QuaternionLookAt(Dir, v3{0,1,0});// * 
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
                ClosestIndex = BoxIndex;
            }
        }

        // TODO(robin) Symbolically solve
        //m3x4 UnscaledBoxTransform = Translation(P) * QuaternionRotationMatrix(Box->Orient);//XYZRotationN(Angles);
        //m3x4 BoxTransform = UnscaledBoxTransform * Scaling(HalfDim);


#if 0
        f32 B = (0.5f*(1.0f + SineApproxN(V)));
        f32 R = (0.5f*(1.0f + SineApproxN(V + 1.0f/3.0f)));
        f32 G = (0.5f*(1.0f + SineApproxN(V + 2.0f/3.0f)));
        //u32 C = RandomSolidColor(&Random);
        u32 C = Pack01RGBA255(R,G,B);
#else
        u32 C = Box->Color;
#endif

        PushBox(&Renderer->Boxes, Box->P, Dim, Box->Orient, C);
        f32 CameraDistance = Length(Camera->P - Box->P);

        if(CameraDistance < 50.0f)
        {
            f32 TextAlpha = 1.0f - SmoothStep(5.0f, CameraDistance, 15.0f);

            m3x4 UnscaledBoxTransform = Translation(Box->P) * QuaternionRotationMatrix(Box->Orient);//XYZRotationN(Angles);
            m3x4 MakeUpright = MatrixAsRows3x4(v3{1,0,0}, v3{0,0,1}, v3{0,1,0});
            // TODO(robin) Symbolically solve
            m3x4 TextTransform = UnscaledBoxTransform *
                                 Translation(-HalfDim.x + Padding.x, -HalfDim.y*1.002f, HalfDim.z - Padding.y) *
                                 MakeUpright;

            u32 Color = Pack01RGBA255(1,1,1, TextAlpha);
            PushText(&Renderer->Text, Box->Text, TextTransform, Color);
        }
    }



    if(MouseLeft->HalfTransitionCount && MouseLeft->EndedDown)
    {
        MouseLeft->HalfTransitionCount = 0;
        State.SelectedDataBox = ClosestIndex;
    }
    

    //m3x4 PlanetTransform = Translation(OrbitCenter) * Scaling(Minimum(Radius*0.1f, 10.0f)) * ZRotationN(Anim[4]);// * Scaling(10.0f);
    //m3x4 InvObjectTransform = ZRotationN(-0.6f) * Scaling(1.0f/10, 1.0f/9, 1.0f/9) * Translation(-5, 0, 0);
    //u32 C = 0xA0808080;
    //PushBox(&State.Default, PlanetTransform, C,C,C,C,C,C);
    //PushBox(Translation(MouseWorldP)*Scaling(0.05f));


    Flush(&Renderer->Boxes);
    Flush(&Renderer->Text);

    glUseProgram(Renderer->Text.Program);
    m4x4 HUDProj = HUDProjection(Width, Height);
    glUniformMatrix4fv(Renderer->Text.Transform, true, &HUDProj);

    char Buf[48];
    string HudText = FormatText(Buf, "%f fps (%f ms)\n"
                                     "%u triangles",
                                     1.0f / DeltaTime, DeltaTime*1000.0f,
                                     LastFrameTriangleCount);

    PushText(&Renderer->Text, HudText, Scaling(0.35f));

    Flush(&Renderer->Text);

    // NOTE(robin): Draw bus mesh
    { 
        glUseProgram(Renderer->Mesh.Program);
        glUniformMatrix4fv(Renderer->Mesh.ViewTransform, true, &RenderTransform.Forward);
        m4x4 ObjectTransform = To4x4(Scaling(5.0f) * ZRotationN(Anim[1]));
        glUniformMatrix4fv(Renderer->Mesh.ObjectTransform, true, &ObjectTransform);
        glUniform3f(Renderer->Mesh.MouseWorldP, HitTest.RayOrigin + HitTest.RayDir*7.5f);
        glBindVertexArray(Renderer->Mesh.VertexArray);
        glDrawElements(GL_TRIANGLES, Renderer->Mesh.IndexCount, GL_UNSIGNED_SHORT, 0);
    }


    // NOTE(robin): Post processing
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(Renderer->PostProcessing.Program);
        glActiveTexture(GL_TEXTURE0+1);
        glBindTexture(GL_TEXTURE_2D, Renderer->FramebufferColorTexture);
        glUniform1i(Renderer->PostProcessing.FramebufferSampler, 1);
        glBindVertexArray(Renderer->RectangleVertexArray);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glActiveTexture(GL_TEXTURE0);
    }

    Reset(&State.FrameMemory);
}


