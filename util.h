
#include <stdarg.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uintptr_t size;

typedef float f32;
typedef double f64;

constexpr u8 U8Max = 0xFF;
constexpr u16 U16Max = 0xFFFF;
constexpr u32 U32Max = 0xFFFFFFFF;
constexpr u64 U64Max = 0xFFFFFFFFFFFFFFFF;
constexpr f32 F32Max = 3.40282347e+38F;


#define OffsetOf(Struct, Member) ((uintptr_t)&((Struct *)0)->Member)

// NOTE(robin): There are many different meanings for static.
// Using these makes it easier to grep the codebase
#define function static
#define local_persist static
#define global static

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

#define S(s) string{sizeof(s)-1,(char *)s}
#define WrapBuf(b) buffer{sizeof(b),b}

#define import_from_js extern "C"
#define export_to_js extern "C" __attribute__((visibility("default")))

#define Likely(Condition)   __builtin_expect(!!(Condition), 1)
#define Unlikely(Condition) __builtin_expect(!!(Condition), 0)

// NOTE(robin): console.log
import_from_js void JS_Log(size StringLength, void *StringPtr);

// NOTE(robin): throw new Error(File + Line + Reason)
import_from_js void JS_Abort(size ReasonLength, void *ReasonPtr, size FileNameLength, void *FileNamePtr, size FuncNameSize, void *FuncNamePtr, u32 LineNumber);
function void JS_Abort(string Reason, string File, string Func, u32 LineNumber)
{
    JS_Abort(Reason.Size, Reason.Contents, File.Size, File.Contents, Func.Size, Func.Contents, LineNumber);
}

#define Assert(Condition) if(Unlikely(!(Condition))) Abort("Assertion failed: " #Condition)
#define Abort(Reason) JS_Abort(S(Reason), S(__FILE__), S(__FUNCTION__), __LINE__)

#define InvalidDefaultCase default: Abort("Invalid default case hit!")
#define NotImplemented Abort("Not implemented!")


function void JS_Log(string String)
{
    JS_Log(String.Size, String.Contents);
}

function f64 AbsoluteValue(f64 V) { return __builtin_fabs(V); };
function s32 AbsoluteValue(s32 V) { return __builtin_abs(V); };
function f32 Square(f32 V) { return V*V; }
function f32 SquareRoot(f32 V) { return __builtin_sqrtf(V); }
function f32 AbsoluteValue(f32 V) { return __builtin_fabsf(V); };
function f32 Floor(f32 V) { return __builtin_floorf(V); };
function f32 Ceil(f32 V) { return __builtin_ceilf(V); };

// TODO(robin): Convince llvm to use the wasm "f32.nearest" instruction here.. (it actually tries linking with roundf if you use __builtin_roundf)
function f32 Round(f32 V) { return Floor(V + 0.5f); };



template<typename number> constexpr
function number Minimum(number A, number B)
{
    return A < B ? A : B;
}
template<typename number> constexpr
function number Maximum(number A, number B)
{
    return A > B ? A : B;
}



function void MemoryCopy(void *Dest, void *Source, size Size)
{
    u8 *Dest8 = (u8 *)Dest;
    u8 *Source8 = (u8 *)Source;
    while((Size % 8) != 0)
    {
        *Dest8++ = *Source8++;
        Size -= 1;
    }
    
    Size /= 8;

    u64 *Dest64 = (u64 *)Dest8;
    u64 *Source64 = (u64 *)Source8;
    while(Size--)
    {
        *Dest64++ = *Source64++;
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

function string U64ToAscii_Dec(string *Dest, u64 Value)
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


function string S64ToAscii_Dec(string *Dest, s64 Value)
{
    bool Negative = Value < 0;

    if(Negative)
    {
        Value = -Value;
        Dest->Contents[0] = '-';
        Advance(Dest);
    }

    string Result = U64ToAscii_Dec(Dest, (u64)Value);

    if(Negative)
    {
        ++Result.Size;
        --Result.Contents;
    }

    return Result;
}

function string U64ToAscii_Hex(string *Dest, u64 Value)
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

#define Printf(Format, ...) Printf_(S(Format), ## __VA_ARGS__)
function void Printf_(string Format, ...);

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
                    U64ToAscii_Dec(&Dest, Value);
                } break;

                case 'x':
                {
                    u32 Value = va_arg(Args, u32);
                    U64ToAscii_Hex(&Dest, Value);
                } break;

                case 'i':
                case 'd':
                {
                    s32 Value = va_arg(Args, s32);
                    S64ToAscii_Dec(&Dest, Value);
                } break;

                case 'B': // binary/bits
                {
                    u32 Value = va_arg(Args, u32);
                    for(int i = 31; i >= 0; --i)
                    {
                        Put(Value & (1 << i) ? '1' : '0');
                    }
                } break;

                case 'f':
                {
                    // TODO(robin): NaN, denormals, etc
                    f64 Value = va_arg(Args, f64);
#if 0
                    NotImplemented;

                    u64 Bits = *(u64 *)&Value;
                    u64 Sign = Bits & ((u64)1 << 63);


                    u64 MantissaMask = (((u64)1 << 52) - 1); 
                    u64 ExponentMask = (((u64)1 << 10) - 1); // TODO(robin): Last bit is assumed to be 1;

                    u64 Mantissa = Bits & MantissaMask;
                    u64 Exponent = (Bits >> 52) & ExponentMask;

                    u64 Upper = Mantissa >> Exponent;
                    u64 Lower = Mantissa & (((u64)1 << (Exponent+1)) - 1);

                    if(Sign)
                    {
                        Put('-');
                    }

                    U64ToAscii_Dec(&Dest, Upper);
                    Put('.');
                    U64ToAscii_Dec(&Dest, Lower);
#else

                    // TODO(robin): Do this properly, this method is not very precise
                    
                    u64 WholePart = (u64)AbsoluteValue(Value);

                    f64 Scale = 100.0;
                    f64 Fract = AbsoluteValue(Value - (f64)WholePart);
                    u64 FractInt = (u64)((Fract * Scale) + 0.5);

                    if(Value < 0.0)
                        Put('-');
                    U64ToAscii_Dec(&Dest, WholePart);
                    Put('.');

                    Scale *= 0.1;
                    while(FractInt < (u64)Scale)
                    {
                        Put('0');
                        Scale *= 0.1;
                    }
                    U64ToAscii_Dec(&Dest, FractInt);
#endif

                } break;

                case 'c':
                {
                    int Value = va_arg(Args, int);

                    if(Dest.Size)
                    {
                        Dest.Contents[0] = (char)Value;
                        Advance(&Dest);
                    }
                } break;

                case 'S':
                {
                    string *String = va_arg(Args, string *);
                    size Count = Minimum(String->Size, Dest.Size);
                    MemoryCopy(Dest.Contents, String->Contents, Count);
                    Advance(&Dest, Count);
                } break;

                InvalidDefaultCase;
            }
        }
    }

    return {DestInit.Size - Dest.Size, DestInit.Contents};
#undef NextChar
#undef Put
}

#define FormatText(Dest, Format, ...) FormatText_(WrapBuf(Dest), S(Format), ## __VA_ARGS__)
function string FormatText_(string Dest, string Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    string Result = FormatText_(Dest, Format, Args);
    va_end(Args);

    return Result;
}

function void Printf_(string Format, ...)
{
    char Buf[256];
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

function void Reset(memory_arena *Memory)
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



//
// https://en.wikipedia.org/wiki/Xorshift
//

struct random_state
{
    u32 x;
};

function u32 xorshift32(random_state *State)
{
	uint32_t x = State->x;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return State->x = x;
}

function random_state DefaultSeed()
{
    random_state Result = {0x12345678};
    return Result;
}

function s32 RandomBetween(random_state *State, s32 Min, s32 Max)
{
    s32 Range = Max - Min;
    s32 Result = Min + (s32)(xorshift32(State) % (u32)Range);
    return Result;
}

function f32 RandomUnilateral(random_state *State)
{
    u32 Random = xorshift32(State);
    f32 Result = Random * (1.0f / (f32)U32Max);
    return Result;
}

function f32 RandomBilateral(random_state *State)
{
    return -1.0f + 2.0f*RandomUnilateral(State);
}


function u32 RandomSolidColor(random_state *Random, u32 Limit = 256)
{
    u32 Result = ((u32)xorshift32(Random) % Limit) << 0 | 
                 ((u32)xorshift32(Random) % Limit) << 8 | 
                 ((u32)xorshift32(Random) % Limit) << 16 |
                 ((u32)0xFF << 24);
    return Result;
}



function u32 Pack01RGBA255(f32 R, f32 G, f32 B, f32 A = 1.0f)
{
    u32 Result = (u32)(R*255.0f + 0.5f) << 0 | 
                 (u32)(G*255.0f + 0.5f) << 8 | 
                 (u32)(B*255.0f + 0.5f) << 16 | 
                 (u32)(A*255.0f + 0.5f) << 24;

    return Result;
}

