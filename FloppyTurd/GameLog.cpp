#include "GameLog.h"
#include "PlatformAPI.h"
#include <cstdio>
#include <cstdarg>

void GameLog::Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    // Create a buffer for the formatted string
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    // Use Raylib's TraceLog instead of printf
    TraceLog(LOG_INFO, buffer);
    
    va_end(args);
}