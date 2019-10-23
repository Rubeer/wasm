

#include "stuff.h"


js_export void Test()
{
    char Buf[64]; string Dest = WrapBuf(Buf);

    for(u32 i = 0; i < 2; ++i)
    {
        string Num = FormatText(Dest, "%u", i);
        Printf("Test: %x, %S", 0xA1B2C3, &Num);
    }
}

extern "C" s32 Sum(s32 *A, u32 Count)
{
    return 0;
}


extern "C" int Add(int a, int b)
{
    return a+b;
}
