
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

export_to_js void AddTestData(u32 NameLength, char *NamePtr,
                              u32 EmailLength, char *EmailPtr,
                              u32 FruitLength, char *FruitPtr)
{
    data_box Box = {};
    Box.Name = {NameLength, NamePtr};
    Box.Email = {EmailLength, EmailPtr};
    Box.Orient = {0, 0, 0, 1};
    
    string Fruit = {FruitLength, FruitPtr};

    for(u32 BoxArrayIndex = 0;
            BoxArrayIndex < State.BoxArrayCount;
            ++BoxArrayIndex)
    {
        data_box_array *Array = State.BoxArrays + BoxArrayIndex;
        if(AreEqual(Array->FavoriteFruit, Fruit))
        {
            Assert(Array->Count < Array->Capacity);
            Array->Data[Array->Count++] = Box;
            return;
        }
    }

    Assert(State.BoxArrayCount < ArrayCount(State.BoxArrays));
    data_box_array *Array = State.BoxArrays + State.BoxArrayCount++;
    Array->FavoriteFruit = Fruit;
    Array->Data[Array->Count++] = Box;
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

    State.SelectedBoxArray = U32Max;
    for(u32 i = 0; i < ArrayCount(State.BoxArrays); ++i)
    {
        data_box_array *Array = State.BoxArrays + i;
        Array->Selected = U32Max;
        Array->Capacity = (1 << 16);
        Array->Data = PushArray(&State.PermanentMemory, data_box, Array->Capacity);
    }
    
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

    glBindFramebuffer(GL_FRAMEBUFFER, Renderer->Framebuffer);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);



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
    u32 ClosestArrayIndex = U32Max;
    f32 ClosestDistance = F32Max;

    glUseProgram(Renderer->Text.Program);
    glUniformMatrix4fv(Renderer->Text.Transform, true, &RenderTransform.Forward);

    glUseProgram(Renderer->Boxes.Program);
    glUniformMatrix4fv(Renderer->Boxes.Transform, true, &RenderTransform.Forward);
    glUniform3f(Renderer->Boxes.MouseWorldP, HitTest.RayOrigin + HitTest.RayDir*7.5f);

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



    random_state Random = DefaultSeed();
    u32 RelBoxIndex = 0;
    u32 AbsoluteBoxIndex = 0;

    for(u32 BoxArrayIndex = 0;
            BoxArrayIndex < State.BoxArrayCount;
            ++BoxArrayIndex)
    {
        data_box_array *Array = State.BoxArrays + BoxArrayIndex;
        v3 PrevP = {};

        f32 Spacing = 0.15f;
        f32 Orbit = ((f32)BoxArrayIndex / (f32)State.BoxArrayCount);

        for(u32 BoxIndex = 0;
                BoxIndex < Array->Count;
                ++BoxIndex, ++AbsoluteBoxIndex)
        {
            data_box *Box = Array->Data  + BoxIndex;

            f32 V = Orbit + ((1.0f - Spacing) * (f32)BoxIndex / (f32)Array->Count) / (f32)State.BoxArrayCount;

            Box->P = OrbitCenter;
            Box->P.x += (0.5f + 0.5f*RandomUnilateral(&Random)) * 100*CosineApproxN(V+Anim[0]);
            Box->P.y += (0.5f + 0.5f*RandomUnilateral(&Random)) * 100*SineApproxN(V+Anim[0]);
            //Box->P.y += (15.0f)*RandomBilateral(&Random) + 60*SineApproxN(V+Anim[0]);
            Box->P.z += CosineApproxN(Anim[0] + V + RandomBilateral(&Random)) * 30.0f;

            v3 Dim = v3{1.9f, 0.9f, 0.9f};
            v3 HalfDim = Dim*0.5f;

            bool Selected = (BoxArrayIndex == State.SelectedBoxArray && BoxIndex == Array->Selected);
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
                // NOTE(robin): Rotate to face forwards on mouse-over
                Box->tSmooth += DeltaTime*2.0f;
                v3 RotUp = {0,1,0};//v3 CamUp = {Camera->Y.y, Camera->Y.z, Camera->Y.x};
                quaternion RotationTarget = QuaternionLookAt(Normalize(Box->P - Camera->P), RotUp);
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
                    ClosestArrayIndex = BoxArrayIndex;
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
            PushBox(&Renderer->Boxes, Box->P, Dim, Box->Orient, C);


            f32 CameraDistance = Length(Camera->P - Box->P);

            if(CameraDistance < 50.0f)
            {
                f32 TextAlpha = 1.0f - SmoothStep(5.0f, CameraDistance, 15.0f);

                m3x4 UnscaledBoxTransform = Translation(Box->P) * QuaternionRotationMatrix(Box->Orient);//XYZRotationN(Angles);
                m3x4 MakeUpright = MatrixAsRows3x4(v3{1,0,0}, v3{0,0,1}, v3{0,1,0});
                // TODO(robin) Symbolically solve
                m3x4 TextTransform = UnscaledBoxTransform * Translation(-HalfDim.x, -HalfDim.y*1.002f, HalfDim.z) * MakeUpright;
                char Buf[128];
                string Text = FormatText(Buf, "%S\n%S\n%S", &Box->Name, &Box->Email, &Array->FavoriteFruit);

                u32 Color = Pack01RGBA255(1,1,1, TextAlpha);
                PushText(&Renderer->Text, Text, TextTransform, Color);
            }
        }


        RelBoxIndex += Array->Count;
    }

    if(MouseLeft->HalfTransitionCount && MouseLeft->EndedDown)
    {
        MouseLeft->HalfTransitionCount = 0;
        State.SelectedBoxArray = ClosestArrayIndex;
        State.BoxArrays[State.SelectedBoxArray].Selected = ClosestIndex;
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

    glUseProgram(Renderer->Mesh.Program);
    glUniformMatrix4fv(Renderer->Mesh.ViewTransform, true, &RenderTransform.Forward);
    m4x4 ObjectTransform = To4x4(Scaling(5.0f) * ZRotationN(Anim[1]));
    glUniformMatrix4fv(Renderer->Mesh.ObjectTransform, true, &ObjectTransform);
    glUniform3f(Renderer->Mesh.MouseWorldP, HitTest.RayOrigin + HitTest.RayDir*7.5f);
    glBindVertexArray(Renderer->Mesh.VertexArray);
    glDrawElements(GL_TRIANGLES, Renderer->Mesh.IndexCount, GL_UNSIGNED_SHORT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(Renderer->PostProcessing.Program);
    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D, Renderer->FramebufferColorTexture);
    glUniform1i(Renderer->PostProcessing.FramebufferSampler, 1);
    glBindVertexArray(Renderer->PostProcessing.VertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glActiveTexture(GL_TEXTURE0);

    Reset(&State.FrameMemory);
}


