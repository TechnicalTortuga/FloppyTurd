#include "GameLog.h"
#include "LogManager.h"
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

    // Use LogManager for file logging
    std::string cppMessage = [message UTF8String];
    LogManager::GetInstance().Log(cppMessage, "GAME");

    // Alternative: Use NSLogv if you prefer
    // NSString* fullFormat = [@"[GAME] " stringByAppendingString:format];
    // NSLogv(fullFormat, args);

#ifdef PLATFORM_IOS
    // Also append to /tmp/floppyturd_debug.txt
    @try {
        NSString* logLine = [NSString stringWithFormat:@"[GameLog] %@\n", message];
        NSData* data = [logLine dataUsingEncoding:NSUTF8StringEncoding];
        NSFileManager* fm = [NSFileManager defaultManager];
        NSString* path = @"/tmp/floppyturd_debug.txt";
        if (![fm fileExistsAtPath:path]) {
            [data writeToFile:path atomically:YES];
        } else {
            NSFileHandle* fh = [NSFileHandle fileHandleForWritingAtPath:path];
            if (fh) {
                [fh seekToEndOfFile];
                [fh writeData:data];
                [fh closeFile];
            }
        }
    } @catch (NSException* e) {
        NSLog(@"[GameLog] Failed to write to debug file: %@", e);
    }
#endif

    va_end(args);
}
