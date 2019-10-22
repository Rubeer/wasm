

#include "stuff.h"


js_export void Test()
{
    for(u32 i = 0; i < 2; ++i)
    {
        string String = S("dfghkjghsdf");
        Printf("s: %u %u", String.Size, (u32)String.Contents);
        Printf("Hello there. %S", String);
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
