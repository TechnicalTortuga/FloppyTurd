#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>

class LogManager {
public:
    static LogManager& GetInstance();
    
    // Initialize logging system
    void Initialize();
    
    // Log a message with pattern detection
    void Log(const std::string& message, const std::string& category = "INFO");
    
    // Get current log file path
    std::string GetCurrentLogPath() const;
    
    // Clean up old log files (keep only last N files)
    void CleanupOldLogs(int keepCount = 10);
    
private:
    LogManager() = default;
    ~LogManager() = default;
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    
    // Pattern detection
    struct LogPattern {
        std::string pattern;
        int count;
        std::string lastMessage;
        double lastTime;
    };
    
    std::string GenerateLogFileName() const;
    std::string GetCurrentTimestamp() const;
    std::string DetectPattern(const std::string& message);
    void WriteToFile(const std::string& message);
    void HandlePatternRepetition(const LogPattern& pattern);
    
    std::string m_logDir;
    std::string m_currentLogFile;
    std::map<std::string, LogPattern> m_patterns;
    std::mutex m_mutex;
    bool m_initialized = false;
    
    // Configuration
    static constexpr int MAX_FILE_SIZE_MB = 10;
    static constexpr int MAX_PATTERN_REPETITIONS = 100;
    static constexpr double PATTERN_TIMEOUT_SECONDS = 5.0;
};

#endif // LOGMANAGER_H 