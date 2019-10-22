
#include <stdarg.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef char s8;
typedef short s16;
typedef int s32;

struct buffer
{
    u32 Size;
    char *Contents;
};
typedef buffer string;

#define S(s) string{ sizeof(s) - 1, s }

// NOTE(robin): C++ calls JS
#define js_import extern "C" 

// NOTE(robin): JS calls C++
#define js_export extern "C" __attribute__((visibility("default")))

js_import void JSLog(void *Data, s32 Size);

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

void MemoryCopy(void *Dest, void *Source, u32 Size)
{
    u32 *Dest32 = (u32 *)Dest;
    u32 *Source32 = (u32 *)Source;
    while(Size >= 4)
    {
        *Dest32++ = *Source32++;
        Size -= sizeof(u32);
    }

    u8 *Dest8 = (u8 *)Dest32;
    u8 *Source8 = (u8 *)Source32;
    while(Size)
    {
        *Dest8++ = *Source8++;
        Size -= sizeof(u8);
    }
}

void Log(string String)
{
    JSLog(String.Contents, (s32)String.Size);
}

void Advance(string *String, u32 Count = 1)
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
void Printf_(string Format, ...);
#define Printf(Format, ...) Printf_(S(Format), ## __VA_ARGS__)

string FormatText(string Dest, string Format, va_list Args)
{

    #define NextChar(Var)               \
    {                                   \
        if(!Format.Size) return Written; \
        Var = Format.Contents[0];       \
        Advance(&Format);               \
    }
    
    #define Put(Var)                                 \
    {                                                \
        if(Written.Size >= Dest.Size) return Written;  \
        Written.Contents[Written.Size++] = Var;        \
    }

    string Written = {0, Dest.Contents};

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
                    string Number = {0, Written.Contents + Written.Size};

                    u32 Value = va_arg(Args, u32);
                    do
                    {
                        ++Number.Size;
                        Put('0' + (Value % 10));
                        Value /= 10;
                    }
                    while(Value);

                    Mirror(Number);

                } break;

                case 'S':
                {
                    string String = va_arg(Args, string);
                    Printf("Ptr: %u Size: %u", (u32)String.Contents, String.Size);

                    u32 DestRemaining = Dest.Size - Written.Size;
                    u32 Count = Minimum(String.Size, DestRemaining);
                    MemoryCopy(Written.Contents + Written.Size, String.Contents, Count);
                    Written.Size += Count;
                } break;
            }
        }
    }

    return Written;
#undef NextChar
#undef Put
}

string FormatText(string Dest, string Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    string Result = FormatText(Dest, Format, Args);
    va_end(Args);

    return Result;
}

void Printf_(string Format, ...)
{
    char Buf[128];
    string Dest = {sizeof(Buf), Buf};

    va_list Args;
    va_start(Args, Format);
    string Result = FormatText(Dest, Format, Args);
    va_end(Args);

    Log(Result);
}

