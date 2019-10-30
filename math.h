
function f32 Square(f32 V) { return V*V; }
function f32 SquareRoot(f32 V) { return __builtin_sqrtf(V); }
function s32 Abs(s32 V) { return __builtin_abs(V); }

function f32 SmoothCurve010(f32 x)
{
    x = Clamp01(x);
    f32 y = 16.0f*Square(1.0f - x)*x*x; // NOTE(robin): http://thetamath.com/app/y=16(x-1)%5E(2)x%5E(2)
    return y;
}

union v2
{
    struct
    {
        f32 x;
        f32 y;
    };
    f32 E[2];
};


function v2 operator+(v2 A, v2 B) { return {A.x+B.x, A.y+B.y}; }
function v2 operator-(v2 A, v2 B) { return {A.x-B.x, A.y-B.y}; }
function v2 operator*(v2 A, v2 B) { return {A.x*B.x, A.y*B.y}; }
function v2 operator/(v2 A, v2 B) { return {A.x/B.x, A.y/B.y}; }

// NOTE(robin): We don't use the above operators in the below code yet, so that the compiler
// has a better chance of inlining everything properly.
function v2 &operator+=(v2 &A, v2 B) { A = {A.x+B.x, A.y+B.y}; return A; }
function v2 &operator-=(v2 &A, v2 B) { A = {A.x-B.x, A.y-B.y}; return A; }
function v2 &operator*=(v2 &A, v2 B) { A = {A.x*B.x, A.y*B.y}; return A; }
function v2 &operator/=(v2 &A, v2 B) { A = {A.x/B.x, A.y/B.y}; return A; }

function v2 operator*(v2 A, f32 B) { return {A.x*B, A.y*B}; }
function v2 operator*(f32 B, v2 A) { return {A.x*B, A.y*B}; }
function v2 operator/(v2 A, f32 B) { return {A.x/B, A.y/B}; }

function f32 Dot(v2 A, v2 B)     { return A.x*B.x + A.y*B.y; }
function f32 LengthSquared(v2 A) { return A.x*A.x + A.y*A.y; }
// Same here, we purposefully _don't_ use LengthSquared here
function f32 Length(v2 A) { return SquareRoot(A.x*A.x + A.y*A.y); }

function v2 Lerp(v2 A, f32 t, v2 B)
{
    v2 Result;
    Result.x = (1.0f - t)*A.x + t*B.x;
    Result.y = (1.0f - t)*A.y + t*B.y;
    return Result;
}

function v2 Normalize(v2 A)
{
    return A / Length(A);
}

union v3
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
    };

    struct
    {
        f32 r;
        f32 g;
        f32 b;
    };

    struct
    {
        v2 xy;
        f32 IgnoreZ_;
    };

    f32 E[3];
};

function v3 operator+(v3 A, v3 B) { return {A.x+B.x, A.y+B.y, A.z+B.z}; }
function v3 operator-(v3 A, v3 B) { return {A.x-B.x, A.y-B.y, A.z-B.z}; }
function v3 operator*(v3 A, v3 B) { return {A.x*B.x, A.y*B.y, A.z*B.z}; }
function v3 operator/(v3 A, v3 B) { return {A.x/B.x, A.y/B.y, A.z/B.z}; }

function v3 &operator+=(v3 &A, v3 B) { A = {A.x+B.x, A.y+B.y, A.z+B.z}; return A; }
function v3 &operator-=(v3 &A, v3 B) { A = {A.x-B.x, A.y-B.y, A.z-B.z}; return A; }
function v3 &operator*=(v3 &A, v3 B) { A = {A.x*B.x, A.y*B.y, A.z*B.z}; return A; }
function v3 &operator/=(v3 &A, v3 B) { A = {A.x/B.x, A.y/B.y, A.z/B.z}; return A; }

function v3 operator*(v3 A, f32 B) { return {A.x*B, A.y*B, A.z*B}; }
function v3 operator*(f32 B, v3 A) { return {A.x*B, A.y*B, A.z*B}; }
function v3 operator/(v3 A, f32 B) { f32 Div = (1.0f/B); return { A.x*Div, A.y*Div, A.z*Div }; }

function f32 Dot(v3 A, v3 B)     { return A.x*B.x + A.y*B.y + A.z*B.z; }
function f32 LengthSquared(v3 A) { return A.x*A.x + A.y*A.y + A.z*A.z; }
function f32 Length(v3 A) { return SquareRoot(A.x*A.x + A.y*A.y + A.z*A.z); }

function 
v3 Lerp(v3 A, f32 t, v3 B)
{
    v3 Result;
    Result.x = (1.0f - t)*A.x + t*B.x;
    Result.y = (1.0f - t)*A.y + t*B.y;
    Result.z = (1.0f - t)*A.z + t*B.z;
    return Result;
}

function 
v3 Cross(v3 A, v3 B)
{
    v3 Result;
    Result.x = A.y*B.z - A.z*B.y;
    Result.y = A.z*B.x - A.x*B.z;
    Result.z = A.x*B.y - A.y*B.x;
    return Result;
}

function v3 Normalize(v3 A)
{
    return A / Length(A);
}

union v4
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };

    struct
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };

    struct
    {
        v3 xyz;
        f32 IgnoreW_;
    };

    struct
    {
        v2 xy;
        v2 zw;
    };

    f32 E[4];
};

function v4 operator+(v4 A, v4 B) { return {A.x+B.x, A.y+B.y, A.z+B.z, A.w+B.w}; }
function v4 operator-(v4 A, v4 B) { return {A.x-B.x, A.y-B.y, A.z-B.z, A.w-B.w}; }
function v4 operator*(v4 A, v4 B) { return {A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w}; }
function v4 operator/(v4 A, v4 B) { return {A.x/B.x, A.y/B.y, A.z/B.z, A.w/B.w}; }

function v4 &operator+=(v4 &A, v4 B) { A = {A.x+B.x, A.y+B.y, A.z+B.z, A.w+B.w}; return A; }
function v4 &operator-=(v4 &A, v4 B) { A = {A.x-B.x, A.y-B.y, A.z-B.z, A.w-B.w}; return A; }
function v4 &operator*=(v4 &A, v4 B) { A = {A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w}; return A; }
function v4 &operator/=(v4 &A, v4 B) { A = {A.x/B.x, A.y/B.y, A.z/B.z, A.w/B.w}; return A; }

function v4 operator*(v4 A, f32 B) { return {A.x*B, A.y*B, A.z*B, A.w*B}; }
function v4 operator*(f32 B, v4 A) { return {A.x*B, A.y*B, A.z*B, A.w*B}; }
function v4 operator/(v4 A, f32 B) { f32 Div = (1.0f/B); return { A.x*Div, A.y*Div, A.z*Div, A.w*Div }; }

function f32 Dot(v4 A, v4 B)     { return A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w; }
function f32 LengthSquared(v4 A) { return A.x*A.x + A.y*A.y + A.z*A.z + A.w*A.w; }
function f32 Length(v4 A) { return SquareRoot(A.x*A.x + A.y*A.y + A.z*A.z + A.w*A.w); }

function 
v4 Lerp(v4 A, f32 t, v4 B)
{
    v4 Result;
    Result.x = (1.0f - t)*A.x + t*B.x;
    Result.y = (1.0f - t)*A.y + t*B.y;
    Result.z = (1.0f - t)*A.z + t*B.z;
    Result.w = (1.0f - t)*A.w + t*B.w;
    return Result;
}

function v4 Normalize(v4 A)
{
    return A / Length(A);
}


struct m4x4
{
    f32 E[4][4];

};

static constexpr m4x4 IdentityMatrix =
{
    {{1,0,0,0},
     {0,1,0,0},
     {0,0,1,0},
     {0,0,0,1}}
};

struct m4x4_inv
{
    m4x4 Forward;
    m4x4 Inverse;
};

function m4x4 operator*(m4x4 const &A, m4x4 const &B)
{
    m4x4 Result;
    for(int r = 0; r < 4; ++r)
    {
        for(int c = 0; c < 4; ++c)
        {
            Result.E[r][c]  = A.E[r][0] * B.E[0][c];
            Result.E[r][c] += A.E[r][1] * B.E[1][c];
            Result.E[r][c] += A.E[r][2] * B.E[2][c];
            Result.E[r][c] += A.E[r][3] * B.E[3][c];
        }
    }
    return Result;
}

function v4 operator*(m4x4 const &M, v4 V)
{
    v4 Result;
    Result.x = V.x*M.E[0][0] + V.y*M.E[0][1] + V.z*M.E[0][2] + V.w*M.E[0][3];
    Result.y = V.x*M.E[1][0] + V.y*M.E[1][1] + V.z*M.E[1][2] + V.w*M.E[1][3];
    Result.z = V.x*M.E[2][0] + V.y*M.E[2][1] + V.z*M.E[2][2] + V.w*M.E[2][3];
    Result.w = V.x*M.E[3][0] + V.y*M.E[3][1] + V.z*M.E[3][2] + V.w*M.E[3][3];
    return Result;
}
function v3 operator*(m4x4 const &M, v3 V)
{
    v3 Result;
    Result.x = V.x*M.E[0][0] + V.y*M.E[0][1] + V.z*M.E[0][2] + M.E[0][3];
    Result.y = V.x*M.E[1][0] + V.y*M.E[1][1] + V.z*M.E[1][2] + M.E[1][3];
    Result.z = V.x*M.E[2][0] + V.y*M.E[2][1] + V.z*M.E[2][2] + M.E[2][3];
    return Result;
}

function m4x4 Scale(f32 x, f32 y, f32 z)
{
    return
    {
        {{x,0,0,0},
         {0,y,0,0},
         {0,0,z,0},
         {0,0,0,1}}
    };
}

function m4x4 Translation(f32 x, f32 y, f32 z)
{
    return
    {
        {{1,0,0,x},
         {0,1,0,y},
         {0,0,1,z},
         {0,0,0,1}}
    };
}

function m4x4 MatrixAsRows(v3 X, v3 Y, v3 Z)
{
    return
    {
        {{X.x, X.y, X.z, 0},
         {Y.x, Y.y, Y.z, 0},
         {Z.x, Z.y, Z.z, 0},
         {  0,   0,   0, 1}}
    };
}
function m4x4 MatrixAsColumns(v3 X, v3 Y, v3 Z)
{
    return
    {
        {{X.x, Y.x, Z.x, 0},
         {X.y, Y.y, Z.y, 0},
         {X.z, Y.z, Z.z, 0},
         {  0,   0,   0, 1}}
    };
}

function m4x4_inv
PerspectiveProjectionTransform(u32 Width, u32 Height, f32 NearClip, f32 FarClip, f32 FocalLength)
{

    f32 n = NearClip;
    f32 f = FarClip;

    f32 x = FocalLength;
    f32 y = FocalLength;

    if(Width > Height)
        x *= (f32)Height / (f32)Width;
    else
        y *= (f32)Width  / (f32)Height;
    
    f32 a = (n + f) / (n - f);
    f32 b = (n*f*2) / (n - f);
    
    m4x4_inv Result;
    Result.Forward = 
    {
        {{ x,  0,  0,  0},
         { 0,  y,  0,  0},
         { 0,  0,  a,  b},
         { 0,  0, -1,  0}}
    };
    Result.Inverse =
    {
        {{1/x,     0,    0,    0},
         {  0,   1/y,    0,    0},
         {  0,     0,    0,   -1},
         {  0,     0,  1/b,  a/b}}
    };
    
    return Result;
}

function m4x4_inv
CameraTransform(v3 X, v3 Y, v3 Z, v3 Position)
{
    m4x4_inv Result;
    Result.Forward = MatrixAsRows(X, Y, Z);
    v3 Translate = -1.0f * (Result.Forward * Position);
    Result.Forward.E[0][3] += Translate.x;
    Result.Forward.E[1][3] += Translate.y;
    Result.Forward.E[2][3] += Translate.z;

    v3 InvX = X / LengthSquared(X);
    v3 InvY = Y / LengthSquared(Y);
    v3 InvZ = Z / LengthSquared(Z);

    Result.Inverse = MatrixAsColumns(InvX, InvY, InvZ);
    v3 InvP = MatrixAsColumns(InvX, InvY, InvZ) * Translate;

    Result.Inverse.E[0][3] -= InvP.x;
    Result.Inverse.E[1][3] -= InvP.y;
    Result.Inverse.E[2][3] -= InvP.z;

    return Result;
}
