/**
 * @file ConsoleLogHandler.cpp
 * @brief Implementation of desktop console log handler
 */

#include "ConsoleLogHandler.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
#endif

namespace Gnosis {

    ConsoleLogHandler::ConsoleLogHandler(bool useColors, bool logToFile, const std::string& logFilePath)
        : m_logLevel(LogLevel::INFO)
        , m_useColors(useColors)
        , m_logToFile(logToFile)
        , m_logFilePath(logFilePath) {
        
        // On Windows, check if we can enable ANSI color codes
#ifdef _WIN32
        if (m_useColors) {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
#endif
    }

    ConsoleLogHandler::~ConsoleLogHandler() throw() {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }

    bool ConsoleLogHandler::Initialize() {
        if (m_logToFile && !m_logFilePath.empty()) {
            m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
            if (!m_logFile.is_open()) {
                std::cerr << "[GNLog] Failed to open log file: " << m_logFilePath << std::endl;
                return false;
            }
            
            // Write initialization marker
            m_logFile << "\n=== GNLog Session Started ===\n";
            m_logFile.flush();
        }
        
        return true;
    }

    void ConsoleLogHandler::Shutdown() {
        if (m_logFile.is_open()) {
            m_logFile << "=== GNLog Session Ended ===\n\n";
            m_logFile.flush();
            m_logFile.close();
        }
    }

    void ConsoleLogHandler::WriteLog(const LogMessage& message) {
        if (message.level < m_logLevel) {
            return;
        }
        
        std::string formattedMessage = FormatMessage(message);
        
        // Output to console
        if (message.level >= LogLevel::ERROR) {
            // Errors go to stderr
            if (m_useColors) {
                std::cerr << GetColorCode(message.level) << formattedMessage << GetResetCode() << std::endl;
            } else {
                std::cerr << formattedMessage << std::endl;
            }
        } else {
            // Info, debug, etc. go to stdout
            if (m_useColors) {
                std::cout << GetColorCode(message.level) << formattedMessage << GetResetCode() << std::endl;
            } else {
                std::cout << formattedMessage << std::endl;
            }
        }
        
        // Output to file (without colors)
        if (m_logFile.is_open()) {
            m_logFile << formattedMessage << std::endl;
        }
    }

    void ConsoleLogHandler::Flush() {
        std::cout.flush();
        std::cerr.flush();
        
        if (m_logFile.is_open()) {
            m_logFile.flush();
        }
    }

    void ConsoleLogHandler::SetLogLevel(LogLevel level) {
        m_logLevel = level;
    }

    LogLevel ConsoleLogHandler::GetLogLevel() const {
        return m_logLevel;
    }

    void ConsoleLogHandler::SetFileLogging(bool enabled, const std::string& filePath) {
        if (!enabled && m_logFile.is_open()) {
            m_logFile.close();
        }
        
        m_logToFile = enabled;
        if (!filePath.empty()) {
            m_logFilePath = filePath;
        }
        
        if (enabled && !m_logFile.is_open() && !m_logFilePath.empty()) {
            m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
        }
    }

    std::string ConsoleLogHandler::GetColorCode(LogLevel level) const {
        if (!m_useColors) {
            return "";
        }
        
        switch (static_cast<int>(level)) {
            case static_cast<int>(LogLevel::TRACE): return "\033[37m";      // White
            case static_cast<int>(LogLevel::DEBUG): return "\033[36m";      // Cyan
            case static_cast<int>(LogLevel::INFO):  return "\033[32m";      // Green
            case static_cast<int>(LogLevel::WARN):  return "\033[33m";      // Yellow
            case static_cast<int>(LogLevel::ERROR): return "\033[31m";      // Red
            case static_cast<int>(LogLevel::FATAL): return "\033[35;1m";    // Bright Magenta
            default: return "";
        }
    }

    std::string ConsoleLogHandler::GetResetCode() const {
        return m_useColors ? "\033[0m" : "";
    }

    std::string ConsoleLogHandler::FormatMessage(const LogMessage& message) const {
        std::ostringstream oss;
        
        // Format: [TIMESTAMP] [LEVEL] [CATEGORY] Message (file:line)
        oss << "[" << FormatTimestamp(message.timestamp) << "] ";
        oss << "[" << std::setw(5) << LogLevelToString(message.level) << "] ";
        oss << "[" << std::setw(8) << message.category << "] ";
        oss << message.message;
        
        // Add file and line info for debug builds or error levels
        if (!message.file.empty() && (message.level >= LogLevel::ERROR || message.level == LogLevel::DEBUG)) {
            // Extract just the filename from the full path
            std::string filename = message.file;
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }
            
            oss << " (" << filename;
            if (message.line > 0) {
                oss << ":" << message.line;
            }
            if (!message.function.empty()) {
                oss << " in " << message.function;
            }
            oss << ")";
        }
        
        return oss.str();
    }

    std::string ConsoleLogHandler::FormatTimestamp(uint64_t timestamp) const {
        // Convert milliseconds to time_t
        time_t seconds = static_cast<time_t>(timestamp / 1000);
        int milliseconds = static_cast<int>(timestamp % 1000);
        
        // Format time
        std::tm* timeinfo = std::localtime(&seconds);
        std::ostringstream oss;
        oss << std::put_time(timeinfo, "%H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << milliseconds;
        
        return oss.str();
    }

} // namespace Gnosis