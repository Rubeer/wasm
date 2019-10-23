

#include "stuff.h"

void StackTest(u32 Depth = 0)
{
    if((Depth % 8) == 0)
        Printf("%u %u", Depth, &Depth);
    StackTest(Depth + 1);
}

js_export void Init(size InitMemorySize)
{
    Printf("Stack: %u", &InitMemorySize);
    Printf("main.wasm startup");

    static u8 TestMemory[1024*1024];
    Printf("Mem: %u", TestMemory);

    memory_arena Memory = MemoryArenaFromByteArray(TestMemory);
    //StackTest();
}

