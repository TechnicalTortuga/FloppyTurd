#include "GameLog.h"
#include <cstdarg>
#include <cstdio>

#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    #ifdef __OBJC__
        #import <Foundation/Foundation.h>
    #endif
#endif

void GameLog::Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE && defined(__OBJC__)
    NSString* format = [NSString stringWithUTF8String:fmt];
    NSLogv([@"[GAME] " stringByAppendingString:format], args);
#else
    printf("[GAME] ");
    vprintf(fmt, args);
    printf("\n");
#endif

    va_end(args);
}
