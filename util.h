
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

#define S(s) string{sizeof(s)-1, (char *)s}
#define WrapBuf(b) buffer{sizeof(b), (char *)b}

#define import_from_js extern "C"
#define export_to_js extern "C" __attribute__((visibility("default")))

#define Likely(Condition)   __builtin_expect(!!(Condition), 1)
#define Unlikely(Condition) __builtin_expect(!!(Condition), 0)

#define Assert(Condition) if(Unlikely(!(Condition))) Abort("Assertion failed: " #Condition)
#define Abort(Reason) JS_Abort(S(Reason), S(__FILE__), S(__FUNCTION__), __LINE__)

#define InvalidDefaultCase default: Abort("Invalid default case hit!")
#define NotImplemented Abort("Not implemented!")

import_from_js void JS_Log(size StringLength, void *StringPtr);
import_from_js void JS_Abort(size ReasonLength, void *ReasonPtr, size FileNameLength, void *FileNamePtr, size FuncNameSize, void *FuncNamePtr, u32 LineNumber);
function void JS_Abort(string Reason, string File, string Func, u32 LineNumber)
{
    JS_Abort(Reason.Size, Reason.Contents, File.Size, File.Contents, Func.Size, Func.Contents, LineNumber);
}

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
constexpr size WASM_PAGE_SIZE = (1 << 16);




struct random_state
{
    u32 x;
};

