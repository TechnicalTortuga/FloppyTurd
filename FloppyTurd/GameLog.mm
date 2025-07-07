#include "GameLog.h"
#include <cstdarg>
#include <cstdio>
#import <Foundation/Foundation.h>
#import <os/log.h> // Optional: for modern logging

void GameLog::Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // Debug: Confirm the method is called
    fprintf(stderr, "[GameLog] Log called with format: %s\n", fmt);

    // Validate format string
    NSString* format = [NSString stringWithUTF8String:fmt];
    if (format == nil) {
        NSLog(@"[GAME] Error: Invalid UTF-8 format string");
        va_end(args);
        return;
    }

    // Use os_log for better iOS integration (recommended)
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    os_log(OS_LOG_DEFAULT, "[GAME] %@", message);

    // Alternative: Use NSLogv if you prefer
    // NSString* fullFormat = [@"[GAME] " stringByAppendingString:format];
    // NSLogv(fullFormat, args);

    va_end(args);
}
