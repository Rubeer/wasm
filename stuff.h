
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

#define S(s) string{sizeof(s)-1,s}
#define WrapBuf(b) buffer{sizeof(b),b}

// NOTE(robin): C++ calls JS
#define js_import extern "C" 

// NOTE(robin): JS calls C++
#define js_export extern "C" __attribute__((visibility("default")))

// NOTE(robin): console.log
js_import void JSLog(void *Data, s32 Size);

void JSLog(string String)
{
    JSLog(String.Contents, (s32)String.Size);
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

void MemoryCopy(void *Dest, void *Source, u32 Size)
{
    u8 *Dest8 = (u8 *)Dest;
    u8 *Source8 = (u8 *)Source;
    while(Size--)
    {
        *Dest8++ = *Source8++;
    }
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
                    u32 Count = Minimum(String->Size, Dest.Size);
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

    JSLog(Result);
}

