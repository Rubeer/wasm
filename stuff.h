
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


struct buffer
{
    size Size;
    char *Contents;
};
typedef buffer string;

#define S(s) string{sizeof(s)-1,s}
#define WrapBuf(b) buffer{sizeof(b),b}

#define OffsetOf(Struct, Member) ((uintptr_t)&((Struct *)0)->Member)


// NOTE(robin): C++ calls JS
#define js_import extern "C" 

// NOTE(robin): JS calls C++
#define js_export extern "C" __attribute__((visibility("default")))

// NOTE(robin): console.log
js_import void JS_Log(size StringLength, void *StringPtr);

// NOTE(robin): throw new Error(File + Line + Reason)
js_import void JS_Abort(size ReasonLength, void *ReasonPtr,
                       size FileNameLength, void *FileNamePtr,
                       u32 LineNumber);


void JS_Abort(string Reason, string FileName, u32 LineNumber)
{
    JS_Abort(Reason.Size, Reason.Contents, FileName.Size, FileName.Contents, LineNumber);
}

#define Assert(Condition) if(!(Condition)) JS_Abort(S("Assertion failed: " #Condition), S(__FILE__), __LINE__)

void JS_Log(string String)
{
    JS_Log(String.Size, String.Contents);
}

template<typename number>
number Minimum(number A, number B)
{
    return A < B ? A : B;
}
template<typename number>
number Maximum(number A, number B)
{
    return A > B ? A : B;
}

void MemoryCopy(void *Dest, void *Source, size Size)
{
    u8 *Dest8 = (u8 *)Dest;
    u8 *Source8 = (u8 *)Source;
    while(Size--)
    {
        *Dest8++ = *Source8++;
    }
}

void MemorySet(void *Dest, u8 Value, size Size)
{
    u8 *Dest8 = (u8 *)Dest;
	while(Size--)
	{
		*Dest8++ = Value;
	}
}

bool IsPowerOfTwo(u32 V)
{
	return ((V - 1) & V) == 0;
}

void Advance(string *String, size Count = 1)
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

void Mirror(string String)
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

string U32ToAscii_Dec(string *Dest, u32 Value)
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

string U32ToAscii_Hex(string *Dest, u32 Value)
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


string FormatText_(string DestInit, string Format, va_list Args)
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
string FormatText_(string Dest, string Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    string Result = FormatText_(Dest, Format, Args);
    va_end(Args);

    return Result;
}

#define Printf(Format, ...) Printf_(S(Format), ## __VA_ARGS__)
void Printf_(string Format, ...)
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
void *PushSize(memory_arena *Memory, size RequestedSize, arena_push_flags Flags = ArenaFlag_Default, size Alignment = DefaultAlignment)
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

void Clear(memory_arena *Memory)
{
	Memory->Used = 0;
}

#define BootstrapPushStruct(StructType, ArenaName, ...) (StructType *)BootstrapPushStruct_(sizeof(StructType), offsetof(StructType, ArenaName), ## __VA_ARGS__)
void *BootstrapPushStruct_(size StructSize, size ArenaOffset, arena_push_flags Flags = ArenaFlag_Default, size Alignment = DefaultAlignment)
{
    memory_arena Bootstrap = {};
    void *Result = PushSize(&Bootstrap, StructSize, Flags, Alignment);
    MemoryCopy((u8 *)Result + ArenaOffset, &Bootstrap, sizeof(Bootstrap));
    return Result;
}

buffer PushBuffer(memory_arena *Memory, size Size, arena_push_flags Flags = ArenaFlag_Default, size Alignment = DefaultAlignment)
{
    buffer Result;
    Result.Size = Size;
    Result.Contents = (char *)PushSize(Memory, Size, Flags, Alignment);
    return Result;
}

void *PushCopy(memory_arena *Memory, size Size, void *Data, size Alignment = DefaultAlignment)
{
    void *Result = PushSize(Memory, Size, ArenaFlag_NoClear, Alignment);
    MemoryCopy(Result, Data, Size);
    return Result;
}

buffer PushCopy(memory_arena *Memory, buffer Buffer, size Alignment = DefaultAlignment)
{
    buffer Result;
    Result.Size = Buffer.Size;
    Result.Contents = (char *)PushCopy(Memory, Buffer.Size, Buffer.Contents, Alignment);
    return Result;
}

buffer PushStringCopy(memory_arena *Memory, buffer Buffer)
{
    buffer Result;
    Result.Size = Buffer.Size;
    Result.Contents = (char *)PushCopy(Memory, Buffer.Size, Buffer.Contents, 1);
    return Result;
}

string Concat(memory_arena *Memory, string A, string B)
{
    size Size = A.Size + B.Size;
    string Result = PushBuffer(Memory, Size, ArenaFlag_NoClear, 1);
    char *At = Result.Contents;

    MemoryCopy(At, A.Contents, A.Size);  At += A.Size;
    MemoryCopy(At, B.Contents, B.Size);  At += B.Size;

    return Result;
}

string
Concat(memory_arena *Memory, string A, string B, string C)
{
    size Size = A.Size + B.Size + C.Size;
    string Result = PushBuffer(Memory, Size, ArenaFlag_NoClear, 1);
    char *At = Result.Contents;

    MemoryCopy(At, A.Contents, A.Size);  At += A.Size;
    MemoryCopy(At, B.Contents, B.Size);  At += B.Size;
    MemoryCopy(At, C.Contents, C.Size);  At += C.Size;

    return Result;
}

string
Concat(memory_arena *Memory, string A, char Separator, string B)
{
    size Size = A.Size + sizeof(Separator) + B.Size;
    string Result = PushBuffer(Memory, Size, ArenaFlag_NoClear, 1);
    char *At = Result.Contents;

    MemoryCopy(At, A.Contents, A.Size);             At += A.Size;
    MemoryCopy(At, &Separator, sizeof(Separator));  At += sizeof(Separator);
    MemoryCopy(At, B.Contents, B.Size);             At += B.Size;

    return Result;
}
