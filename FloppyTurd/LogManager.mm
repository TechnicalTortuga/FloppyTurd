#include "LogManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cstring>

#ifdef PLATFORM_IOS
#include <Foundation/Foundation.h>
#endif

LogManager& LogManager::GetInstance() {
    static LogManager instance;
    return instance;
}

void LogManager::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;
    
#ifdef PLATFORM_IOS
    #if TARGET_IPHONE_SIMULATOR
        // For simulator, use the user's home directory for easy access
        NSString *homeDirectory = NSHomeDirectory();
        NSString *logsPath = [homeDirectory stringByAppendingPathComponent:@"FloppyTurdLogs"];
        m_logDir = [logsPath UTF8String];
        NSLog(@"[LogManager] Using external logs directory: %@", logsPath);
    #else
        // For real devices, use the app's documents directory
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectory = [paths objectAtIndex:0];
        m_logDir = [documentsDirectory UTF8String];
        
        // Create logs subdirectory
        NSString *logsPath = [documentsDirectory stringByAppendingPathComponent:@"logs"];
    #endif

    // Create logs subdirectory if it doesn't exist
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:[NSString stringWithUTF8String:m_logDir.c_str()]]) {
        NSError *error = nil;
        [fileManager createDirectoryAtPath:[NSString stringWithUTF8String:m_logDir.c_str()] withIntermediateDirectories:YES attributes:nil error:&error];
        if (error) {
            NSLog(@"[LogManager] Failed to create logs directory: %@", error);
        }
    }
#else
    // For desktop, use project logs directory
    m_logDir = "logs";
    // Simple directory creation for desktop
    system(("mkdir -p " + m_logDir).c_str());
#endif
    
    // Generate unique log file name
    m_currentLogFile = GenerateLogFileName();
    
    // Write initial log entry
    std::string initMessage = "=== FLOPPYTURD LOG STARTED ===\n";
    initMessage += "Log file: " + m_currentLogFile + "\n";
    initMessage += "Timestamp: " + GetCurrentTimestamp() + "\n";
    initMessage += "=====================================\n\n";
    
    WriteToFile(initMessage);
    m_initialized = true;
}

std::string LogManager::GenerateLogFileName() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    
    return m_logDir + "/floppyturd_" + ss.str() + ".log";
}

std::string LogManager::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void LogManager::Log(const std::string& message, const std::string& category) {
    if (!m_initialized) {
        Initialize();
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Detect patterns
    std::string pattern = DetectPattern(message);
    
    if (!pattern.empty()) {
        // This is a repeated pattern
        auto& logPattern = m_patterns[pattern];
        logPattern.count++;
        logPattern.lastMessage = message;
        logPattern.lastTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
        
        // Only log every 10th repetition or when pattern changes
        if (logPattern.count % 10 != 1) {
            return;
        }
        
        // If we've repeated too many times, report it
        if (logPattern.count >= MAX_PATTERN_REPETITIONS) {
            HandlePatternRepetition(logPattern);
            return;
        }
    }
    
    // Format the log message
    std::string timestamp = GetCurrentTimestamp();
    std::string formattedMessage = "[" + timestamp + "] [" + category + "] " + message + "\n";
    
    WriteToFile(formattedMessage);
}

std::string LogManager::DetectPattern(const std::string& message) {
    // Simple pattern detection: look for messages that are mostly the same
    // This is a basic implementation - you can make it more sophisticated
    
    for (auto& [pattern, logPattern] : m_patterns) {
        // Check if this message is similar to the pattern
        if (message.find(pattern) != std::string::npos || 
            pattern.find(message) != std::string::npos) {
            
            // Check if it's been too long since the last occurrence
            double currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
            
            if (currentTime - logPattern.lastTime < PATTERN_TIMEOUT_SECONDS) {
                return pattern;
            }
        }
    }
    
    // Create a new pattern from this message
    std::string newPattern = message;
    // Truncate long messages for pattern matching
    if (newPattern.length() > 50) {
        newPattern = newPattern.substr(0, 50) + "...";
    }
    
    LogPattern pattern;
    pattern.pattern = newPattern;
    pattern.count = 1;
    pattern.lastMessage = message;
    pattern.lastTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
    
    m_patterns[newPattern] = pattern;
    return "";
}

void LogManager::HandlePatternRepetition(const LogPattern& pattern) {
    std::string warning = "=== PATTERN REPETITION WARNING ===\n";
    warning += "Pattern: " + pattern.pattern + "\n";
    warning += "Repeated " + std::to_string(pattern.count) + " times\n";
    warning += "Last message: " + pattern.lastMessage + "\n";
    warning += "=====================================\n\n";
    
    WriteToFile(warning);
}

void LogManager::WriteToFile(const std::string& message) {
    NSLog(@"[LogManager] WriteToFile: Attempting to write to %s", m_currentLogFile.c_str());
    std::ofstream file(m_currentLogFile, std::ios::app);
    if (file.is_open()) {
        NSLog(@"[LogManager] WriteToFile: Successfully opened file");
        file << message;
        file.flush();
        file.close();
        
        // Simple file size check without std::filesystem
        std::ifstream sizeCheck(m_currentLogFile, std::ios::binary | std::ios::ate);
        if (sizeCheck.is_open()) {
            std::streamsize fileSize = sizeCheck.tellg();
            sizeCheck.close();
            
            // If file is larger than MAX_FILE_SIZE_MB, rotate it
            if (fileSize > MAX_FILE_SIZE_MB * 1024 * 1024) {
                std::string newLogFile = GenerateLogFileName();
                std::string rotationMessage = "=== LOG ROTATION ===\n";
                rotationMessage += "Previous log file: " + m_currentLogFile + "\n";
                rotationMessage += "New log file: " + newLogFile + "\n";
                rotationMessage += "File size exceeded " + std::to_string(MAX_FILE_SIZE_MB) + "MB\n";
                rotationMessage += "==================\n\n";
                
                // Write rotation message to new file
                std::ofstream newFile(newLogFile, std::ios::app);
                if (newFile.is_open()) {
                    newFile << rotationMessage;
                    newFile.close();
                }
                
                m_currentLogFile = newLogFile;
            }
        }
    } else {
        // If we can't write to the file, try to write to a fallback location
        std::string fallbackFile = m_logDir + "/floppyturd_fallback.log";
        std::ofstream fallback(fallbackFile, std::ios::app);
        if (fallback.is_open()) {
            fallback << "[" + GetCurrentTimestamp() + "] [ERROR] Failed to write to " + m_currentLogFile + "\n";
            fallback << message;
            fallback.close();
        }
    }
}

std::string LogManager::GetCurrentLogPath() const {
    return m_currentLogFile;
}

void LogManager::CleanupOldLogs(int keepCount) {
    // Simplified cleanup without std::filesystem
    // For now, just log that cleanup was requested
    std::string cleanupMessage = "=== LOG CLEANUP REQUESTED ===\n";
    cleanupMessage += "Keep count: " + std::to_string(keepCount) + "\n";
    cleanupMessage += "Current log file: " + m_currentLogFile + "\n";
    cleanupMessage += "==============================\n\n";
    
    WriteToFile(cleanupMessage);
} 