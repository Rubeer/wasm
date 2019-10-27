
#include <stdarg.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint64_t u64;

typedef uintptr_t size;

typedef float f32;
typedef double f64;

constexpr u8 U8Max = 0xFF;
constexpr u16 U16Max = 0xFFFF;
constexpr u32 U32Max = 0xFFFFFFFF;
constexpr u64 U64Max = 0xFFFFFFFFFFFFFFFF;


#define OffsetOf(Struct, Member) ((uintptr_t)&((Struct *)0)->Member)

// NOTE(robin): There are many different meanings for static.
// Using these makes it easier to grep the codebase
#define function static
#define local_persist static
#define global static

#define InvalidDefaultCase default: assert(!"Invalid default case hit!")
#define ArrayCount(x) (sizeof(x) / sizeof(*(x)))

#define Kilobytes(x) ((u64)(x) * 1024)
#define Megabytes(x) (Kilobytes(x) * 1024)
#define Gigabytes(x) (Megabytes(x) * 1024)

struct buffer
{
    size Size;
    char *Contents;
};
typedef buffer string;

#define S(s) string{sizeof(s)-1,s}
#define WrapBuf(b) buffer{sizeof(b),b}

#define import_from_js extern "C" 
#define export_to_js extern "C" __attribute__((visibility("default")))

#define Likely(Condition)   __builtin_expect(!!(Condition), 1)
#define Unlikely(Condition) __builtin_expect(!!(Condition), 0)

// NOTE(robin): console.log
import_from_js void JS_Log(size StringLength, void *StringPtr);

// NOTE(robin): throw new Error(File + Line + Reason)
import_from_js void JS_Abort(size ReasonLength, void *ReasonPtr, size FileNameLength, void *FileNamePtr, u32 LineNumber);
function void JS_Abort(string Reason, string FileName, u32 LineNumber)
{
    JS_Abort(Reason.Size, Reason.Contents, FileName.Size, FileName.Contents, LineNumber);
}

#define Assert(Condition) if((!(Condition))) JS_Abort(S("Assertion failed: " #Condition), S(__FILE__), __LINE__)

function void JS_Log(string String)
{
    JS_Log(String.Size, String.Contents);
}


template<typename number>
function number Minimum(number A, number B)
{
    return A < B ? A : B;
}
template<typename number>
function number Maximum(number A, number B)
{
    return A > B ? A : B;
}

function f32 Clamp01(f32 V)
{
    return Maximum(0.0f, Minimum(1.0f, V));
}

function void MemoryCopy(void *Dest, void *Source, size Size)
{
    u8 *Dest8 = (u8 *)Dest;
    u8 *Source8 = (u8 *)Source;
    while(Size--)
    {
        *Dest8++ = *Source8++;
    }
}

function void MemorySet(void *Dest, u8 Value, size Size)
{
    u8 *Dest8 = (u8 *)Dest;
	while(Size--)
	{
		*Dest8++ = Value;
	}
}

function bool IsPowerOfTwo(u32 V)
{
	return ((V - 1) & V) == 0;
}

function void Advance(string *String, size Count = 1)
{
    if(String->Size > Count)
    {
        String->Size -= Count;
        String->Contents += Count;
    }
    else
    {
        *String = {};
    }
}

function void Mirror(string String)
{
    char *Start = String.Contents;
    char *End = String.Contents + String.Size;
    while(Start < End)
    {
        --End;
        char Tmp = *End;
        *End = *Start;
        *Start = Tmp;
        ++Start;
    }
}

function string U32ToAscii_Dec(string *Dest, u32 Value)
{
    string Result = *Dest;

    do
    {
        if(!Dest->Size) break;
        Dest->Contents[0] = '0' + (Value % 10);
        Advance(Dest);
        Value /= 10;
    }
    while(Value);

    Result.Size -= Dest->Size;
    Mirror(Result);

    return Result;
}

function string U32ToAscii_Hex(string *Dest, u32 Value)
{
    string Result = *Dest;

    do
    {
        if(!Dest->Size) break;
        Dest->Contents[0] = "0123456789ABCDEF"[Value % 16];
        Advance(Dest);
        Value /= 16;
    }
    while(Value);

    Result.Size -= Dest->Size;
    Mirror(Result);

    return Result;
}


function string FormatText_(string DestInit, string Format, va_list Args)
{
    string Dest = DestInit;

    #define NextChar(Var)           \
    {                               \
        if(!Format.Size) return {DestInit.Size - Dest.Size, DestInit.Contents};   \
        Var = Format.Contents[0];   \
        Advance(&Format);           \
    }
    
    #define Put(Var)                      \
    {                                     \
        if(!Dest.Size) return DestInit;   \
        Dest.Contents[0] = Var;           \
        Advance(&Dest);                   \
    }


    char Char = 0;
    for(;;)
    {
        NextChar(Char);
        if(Char != '%')
        {
            Put(Char);
        }
        else
        {
            NextChar(Char);
            switch(Char)
            {
                case '%':
                {
                    Put('%');
                } break;

                case 'u':
                {
                    u32 Value = va_arg(Args, u32);
                    U32ToAscii_Dec(&Dest, Value);
                } break;

                case 'x':
                {
                    u32 Value = va_arg(Args, u32);
                    U32ToAscii_Hex(&Dest, Value);
                } break;

                case 'S':
                {
                    string *String = va_arg(Args, string *);
                    size Count = Minimum(String->Size, Dest.Size);
                    MemoryCopy(Dest.Contents, String->Contents, Count);
                    Advance(&Dest, Count);
                } break;
            }
        }
    }

    return {DestInit.Size - Dest.Size, DestInit.Contents};
#undef NextChar
#undef Put
}

#define FormatText(Dest, Format, ...) FormatText_(Dest, S(Format), ## __VA_ARGS__)
function string FormatText_(string Dest, string Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    string Result = FormatText_(Dest, Format, Args);
    va_end(Args);

    return Result;
}

#define Printf(Format, ...) Printf_(S(Format), ## __VA_ARGS__)
function void Printf_(string Format, ...)
{
    char Buf[128];
    string Dest = {sizeof(Buf), Buf};

    va_list Args;
    va_start(Args, Format);
    string Result = FormatText_(Dest, Format, Args);
    va_end(Args);

    JS_Log(Result);
}

struct memory_arena
{
    buffer Buffer;
    size Used;
};

enum arena_push_flags
{
    ArenaFlag_Default = 0,
    ArenaFlag_NoClear = (1 << 0),
};

constexpr size DefaultAlignment = 4;

#define MemoryArenaFromByteArray(Array) memory_arena{buffer{sizeof(Array), (char *)Array}, 0}

#define PushStruct(Memory, Type, ...) ((Type *)PushSize(Memory, sizeof(Type), ## __VA_ARGS__))
#define PushArray(Memory, Type, Count, ...) ((Type *)PushSize(Memory, (sizeof(Type)*(Count)), ## __VA_ARGS__))
#define PushInfer(Memory, Pointer, ...) ((decltype(Pointer))PushSize(Memory, sizeof(*(Pointer)), ## __VA_ARGS__))
function void *PushSize(memory_arena *Memory, size RequestedSize, arena_push_flags Flags = ArenaFlag_Default, size Alignment = DefaultAlignment)
{
    Assert(IsPowerOfTwo(Alignment));

    size AlignmentOffset = 0;
    if(Alignment > 1)
    {
        size Address = (size)Memory->Buffer.Contents + Memory->Used;
        AlignmentOffset = Alignment - (Address & (Alignment - 1));
    }

	size ActualSize = RequestedSize + AlignmentOffset;
	Assert(Memory->Used + ActualSize <= Memory->Buffer.Size);

    u8 *Result = (u8 *)Memory->Buffer.Contents + Memory->Used + AlignmentOffset;
    Memory->Used += ActualSize;


    if(!(Flags & ArenaFlag_NoClear))
    {
        MemorySet(Result, 0, RequestedSize);
    }

    return (void *)Result;
}

function void Clear(memory_arena *Memory)
{
	Memory->Used = 0;
}

#define BootstrapPushStruct(StructType, ArenaName, ...) (StructType *)BootstrapPushStruct_(sizeof(StructType), offsetof(StructType, ArenaName), ## __VA_ARGS__)
function void *BootstrapPushStruct_(size StructSize, size ArenaOffset, arena_push_flags Flags = ArenaFlag_Default, size Alignment = DefaultAlignment)
{
    memory_arena Bootstrap = {};
    void *Result = PushSize(&Bootstrap, StructSize, Flags, Alignment);
    MemoryCopy((u8 *)Result + ArenaOffset, &Bootstrap, sizeof(Bootstrap));
    return Result;
}

function buffer PushBuffer(memory_arena *Memory, size Size, arena_push_flags Flags = ArenaFlag_Default, size Alignment = DefaultAlignment)
{
    buffer Result;
    Result.Size = Size;
    Result.Contents = (char *)PushSize(Memory, Size, Flags, Alignment);
    return Result;
}

function void *PushCopy(memory_arena *Memory, size Size, void *Data, size Alignment = DefaultAlignment)
{
    void *Result = PushSize(Memory, Size, ArenaFlag_NoClear, Alignment);
    MemoryCopy(Result, Data, Size);
    return Result;
}

function buffer PushCopy(memory_arena *Memory, buffer Buffer, size Alignment = DefaultAlignment)
{
    buffer Result;
    Result.Size = Buffer.Size;
    Result.Contents = (char *)PushCopy(Memory, Buffer.Size, Buffer.Contents, Alignment);
    return Result;
}

function buffer PushStringCopy(memory_arena *Memory, buffer Buffer)
{
    return PushCopy(Memory, Buffer, 1);
}

function string Concat(memory_arena *Memory, string A, string B)
{
    size Size = A.Size + B.Size;
    string Result = PushBuffer(Memory, Size, ArenaFlag_NoClear, 1);
    char *At = Result.Contents;

    MemoryCopy(At, A.Contents, A.Size);  At += A.Size;
    MemoryCopy(At, B.Contents, B.Size);  At += B.Size;

    return Result;
}

function string Concat(memory_arena *Memory, string A, string B, string C)
{
    size Size = A.Size + B.Size + C.Size;
    string Result = PushBuffer(Memory, Size, ArenaFlag_NoClear, 1);
    char *At = Result.Contents;

    MemoryCopy(At, A.Contents, A.Size);  At += A.Size;
    MemoryCopy(At, B.Contents, B.Size);  At += B.Size;
    MemoryCopy(At, C.Contents, C.Size);  At += C.Size;

    return Result;
}

function string Concat(memory_arena *Memory, string A, char Separator, string B)
{
    size Size = A.Size + sizeof(Separator) + B.Size;
    string Result = PushBuffer(Memory, Size, ArenaFlag_NoClear, 1);
    char *At = Result.Contents;

    MemoryCopy(At, A.Contents, A.Size);             At += A.Size;
    MemoryCopy(At, &Separator, sizeof(Separator));  At += sizeof(Separator);
    MemoryCopy(At, B.Contents, B.Size);             At += B.Size;

    return Result;
}


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

function m4x4 MatrixAsColumns(v3 X, v3 Y, v3 Z)
{
    return
    {
        {{X.x, X.y, X.z, 0},
         {Y.x, Y.y, Y.z, 0},
         {Z.x, Z.y, Z.z, 0},
         {  0,   0,   0, 1}}
    };
}
function m4x4 MatrixAsRows(v3 X, v3 Y, v3 Z)
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
    Result.Forward = MatrixAsColumns(X, Y, Z);
    v3 Translate = -1.0f * (Result.Forward * Position);
    Result.Forward.E[0][3] += Translate.x;
    Result.Forward.E[1][3] += Translate.y;
    Result.Forward.E[2][3] += Translate.z;

    v3 InvX = X / LengthSquared(X);
    v3 InvY = Y / LengthSquared(Y);
    v3 InvZ = Z / LengthSquared(Z);

    Result.Inverse = MatrixAsColumns(InvX, InvY, InvZ);
    v3 InvP = MatrixAsRows(InvX, InvY, InvZ) * Position;

    Result.Inverse.E[0][3] -= InvP.x;
    Result.Inverse.E[1][3] -= InvP.y;
    Result.Inverse.E[2][3] -= InvP.z;

    return Result;
}



//
// https://en.wikipedia.org/wiki/Xorshift
//

function u64 rol64(u64 x, int k)
{
	return (x << k) | (x >> (64 - k));
}

struct random_state
{
	u64 s[4];
};

function random_state DefaultSeed()
{
    random_state Result = {0x12345678ABCDEF65, 0x12345678ABCDEF65, 0xABCDEF0987654321, 0x1234567812345678};
    return Result;
}

function u32 xoshiro256p(random_state *State)
{
	u64 *s = State->s;
	u64 Result = s[0] + s[3];
	u64 t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;
	s[3] = rol64(s[3], 45);

	return Result >> 32;
}

function f32 RandomUnilateral(random_state *State)
{
    u32 Random = xoshiro256p(State);
    f32 Result = Random * (1.0f / (f32)U32Max);
    return Result;
}

function f32 RandomBilateral(random_state *State)
{
    return -1.0f + 2.0f*RandomUnilateral(State);
}
