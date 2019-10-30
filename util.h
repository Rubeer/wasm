
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

#define Assert(Condition) if(Unlikely(!(Condition))) Abort(S("Assertion failed: " #Condition))
#define Abort(Reason) JS_Abort(Reason, S(__FILE__), S(__FUNCTION__), __LINE__)

#define InvalidDefaultCase default: Abort(S("Invalid default case hit!"))
#define NotImplemented Abort(S("Not implemented!"))


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
                    NotImplemented;

                    // TODO(robin): NaN, denormals, etc
                    f64 Value = va_arg(Args, f64);
                    u64 Bits = *(u64 *)&Value;

                    u64 MantissaMask = (((u64)1 << 52) - 1); 
                    u64 ExponentMask = (((u64)1 << 10) - 1); // TODO(robin): Last bit is assumed to be 1;

                    u64 Mantissa = Bits & MantissaMask;
                    u64 Exponent = (Bits >> 52) & ExponentMask;
                    u64 Sign = Bits & ((u64)1 << 63);

                    u64 Upper = Mantissa >> Exponent;
                    u64 Lower = Mantissa & (((u64)1 << (Exponent+1)) - 1);

                    if(Sign)
                    {
                        Put('-');
                    }

                    U64ToAscii_Dec(&Dest, Upper);
                    Put('.');
                    U64ToAscii_Dec(&Dest, Lower);

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

#define FormatText(Dest, Format, ...) FormatText_(Dest, S(Format), ## __VA_ARGS__)
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
    random_state Result = {0x12345678ABCDEF65, 0x12345678ABCDEF65,
                          0xABCDEF0987654321, 0x1234567812345678};
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
