#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace WebAssets {
inline int File(const char* path) {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({ return window.floppyAssets.file(UTF8ToString($0)); }, path);
#else
    (void)path; return 1;
#endif
}
inline void Release(const char* path) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ window.floppyAssets.release(UTF8ToString($0)); }, path);
#else
    (void)path;
#endif
}
inline int Level(int index) {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({ return window.floppyAssets.group('level-'+$0); }, index);
#else
    (void)index; return 1;
#endif
}
inline void RetryLevel(int index) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ window.floppyAssets.retryGroup('level-'+$0); }, index);
#else
    (void)index;
#endif
}
}
