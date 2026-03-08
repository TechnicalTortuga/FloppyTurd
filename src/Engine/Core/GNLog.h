/**
 * @file GNLog.h
 * @brief Platform-agnostic logging system for Gnosis Engine
 * 
 * This system provides a unified logging interface that routes messages
 * to appropriate platform-specific log managers (iOS Console, Desktop stdout, etc.)
 * 
 * Usage:
 *   GN_LOG_INFO("Player scored: {}", score);
 *   GN_LOG_ERROR("Failed to load texture: {}", filename);
 *   GN_LOG_DEBUG("Position: ({}, {})", x, y);
 */

#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <vector>
#include <mutex>
#include <chrono>

#ifdef PLATFORM_IOS
#include "../../iOS/Threading/ThreadingProxy.h"
#endif

namespace Gnosis {

    /**
     * @enum LogLevel
     * @brief Defines the severity levels for log messages
     */
    enum class LogLevel {
        TRACE = 0,    // Detailed trace information
        DBG = 1,      // Debug information
        INFO = 2,     // General information
        WARN = 3,     // Warning messages
        ERROR = 4,    // Error messages
        FATAL = 5     // Fatal error messages
    };

    /**
     * @struct LogMessage
     * @brief Contains all information about a log message
     */
    struct LogMessage {
        LogLevel level;
        std::string message;
        std::string category;
        std::string file;
        int line;
        std::string function;
        uint64_t timestamp;
        
        LogMessage(LogLevel lvl, const std::string& msg, const std::string& cat = "GAME",
                  const std::string& f = "", int l = 0, const std::string& func = "")
            : level(lvl), message(msg), category(cat), file(f), line(l), function(func) {
            // Get timestamp (milliseconds since epoch)
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        }
    };

    /**
     * @interface ILogHandler
     * @brief Abstract interface for platform-specific log handlers
     */
    class ILogHandler {
    public:
        virtual ~ILogHandler() = default;
        
        /**
         * @brief Initialize the log handler
         * @return true if initialization successful
         */
        virtual bool Initialize() = 0;
        
        /**
         * @brief Shutdown the log handler
         */
        virtual void Shutdown() = 0;
        
        /**
         * @brief Write a log message to the platform-specific output
         * @param message The log message to write
         */
        virtual void WriteLog(const LogMessage& message) = 0;
        
        /**
         * @brief Flush any buffered log messages
         */
        virtual void Flush() = 0;
        
        /**
         * @brief Set the minimum log level for this handler
         * @param level Minimum level to log
         */
        virtual void SetLogLevel(LogLevel level) = 0;
        
        /**
         * @brief Get the current minimum log level
         * @return Current minimum log level
         */
        virtual LogLevel GetLogLevel() const = 0;
    };

    /**
     * @class GNLog
     * @brief Central logging manager that routes messages to platform handlers
     */
    class GNLog {
    private:
        static std::unique_ptr<GNLog> s_instance;
        static std::mutex s_mutex;
        
        std::vector<std::unique_ptr<ILogHandler> > m_handlers;
        LogLevel m_globalLogLevel;
        std::mutex m_logMutex;
        bool m_initialized;
        
        GNLog();
        
    public:
        ~GNLog();
        
        // Singleton access
        static GNLog& GetInstance();
        static void Initialize();
        static void Shutdown();
        
        /**
         * @brief Add a log handler to the system
         * @param handler Platform-specific log handler
         */
        void AddHandler(std::unique_ptr<ILogHandler> handler);
        
        /**
         * @brief Remove all handlers
         */
        void ClearHandlers();
        
        /**
         * @brief Set the global minimum log level
         * @param level Minimum level to log globally
         */
        void SetGlobalLogLevel(LogLevel level);
        
        /**
         * @brief Get the global minimum log level
         * @return Current global minimum log level
         */
        LogLevel GetGlobalLogLevel() const { return m_globalLogLevel; }
        
        /**
         * @brief Log a message with specified level
         * @param level Log level
         * @param message Message to log
         * @param category Category/tag for the message
         * @param file Source file name
         * @param line Source line number
         * @param function Source function name
         */
        void Log(LogLevel level, const std::string& message, const std::string& category = "GAME",
                const std::string& file = "", int line = 0, const std::string& function = "");
        
        /**
         * @brief Check if a log level would be processed
         * @param level Log level to check
         * @return true if the level would be logged
         */
        bool ShouldLog(LogLevel level) const;
        
        /**
         * @brief Flush all handlers
         */
        void FlushAll();
    };

    /**
     * @brief Convert log level to string
     * @param level Log level to convert
     * @return String representation of log level
     */
    const char* LogLevelToString(LogLevel level);
    
    /**
     * @brief Convert string to log level
     * @param str String to convert
     * @return Log level, defaults to INFO if invalid
     */
    LogLevel StringToLogLevel(const std::string& str);

} // namespace Gnosis



// Convenience macros for logging
#ifdef PLATFORM_IOS
    // iOS-specific macros - use std::string with proper formatting
    #define GN_LOG_TRACE(msg, ...) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            GameCore::ThreadingProxy::enqueueLogTrace(oss.str().c_str(), "GAME"); \
        } while(0)

    #define GN_LOG_DEBUG(msg, ...) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            GameCore::ThreadingProxy::enqueueLogDebug(oss.str().c_str(), "GAME"); \
        } while(0)

    #define GN_LOG_INFO(msg, ...) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            GameCore::ThreadingProxy::enqueueLogInfo(oss.str().c_str(), "GAME"); \
        } while(0)

    #define GN_LOG_WARN(msg, ...) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            GameCore::ThreadingProxy::enqueueLogWarn(oss.str().c_str(), "GAME"); \
        } while(0)

    #define GN_LOG_ERROR(msg, ...) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            GameCore::ThreadingProxy::enqueueLogError(oss.str().c_str(), "GAME"); \
        } while(0)

    #define GN_LOG_FATAL(msg, ...) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            GameCore::ThreadingProxy::enqueueLogFatal(oss.str().c_str(), "GAME"); \
        } while(0)
#else
    // Default macros for non-iOS platforms
    #define GN_LOG_TRACE(msg, ...) \
        if (Gnosis::GNLog::GetInstance().ShouldLog(Gnosis::LogLevel::TRACE)) { \
            std::ostringstream oss; \
            oss << msg; \
            Gnosis::GNLog::GetInstance().Log(Gnosis::LogLevel::TRACE, oss.str(), "GAME", __FILE__, __LINE__, __FUNCTION__); \
        }

    #define GN_LOG_DEBUG(msg, ...) \
        if (Gnosis::GNLog::GetInstance().ShouldLog(Gnosis::LogLevel::DBG)) { \
            std::ostringstream oss; \
            oss << msg; \
            Gnosis::GNLog::GetInstance().Log(Gnosis::LogLevel::DBG, oss.str(), "GAME", __FILE__, __LINE__, __FUNCTION__); \
        }

    #define GN_LOG_INFO(msg, ...) \
        if (Gnosis::GNLog::GetInstance().ShouldLog(Gnosis::LogLevel::INFO)) { \
            std::ostringstream oss; \
            oss << msg; \
            Gnosis::GNLog::GetInstance().Log(Gnosis::LogLevel::INFO, oss.str(), "GAME", __FILE__, __LINE__, __FUNCTION__); \
        }

    #define GN_LOG_WARN(msg, ...) \
        if (Gnosis::GNLog::GetInstance().ShouldLog(Gnosis::LogLevel::WARN)) { \
            std::ostringstream oss; \
            oss << msg; \
            Gnosis::GNLog::GetInstance().Log(Gnosis::LogLevel::WARN, oss.str(), "GAME", __FILE__, __LINE__, __FUNCTION__); \
        }

    #define GN_LOG_ERROR(msg, ...) \
        if (Gnosis::GNLog::GetInstance().ShouldLog(Gnosis::LogLevel::ERROR)) { \
            std::ostringstream oss; \
            oss << msg; \
            Gnosis::GNLog::GetInstance().Log(Gnosis::LogLevel::ERROR, oss.str(), "GAME", __FILE__, __LINE__, __FUNCTION__); \
        }

    #define GN_LOG_FATAL(msg, ...) \
        if (Gnosis::GNLog::GetInstance().ShouldLog(Gnosis::LogLevel::FATAL)) { \
            std::ostringstream oss; \
            oss << msg; \
            Gnosis::GNLog::GetInstance().Log(Gnosis::LogLevel::FATAL, oss.str(), "GAME", __FILE__, __LINE__, __FUNCTION__); \
        }
#endif

// Category-specific logging macros
#ifdef PLATFORM_IOS
    // iOS-specific category macros - use std::string with proper formatting
    #define GN_LOG_CATEGORY(level, category, msg, ...) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            switch(level) { \
                case Gnosis::LogLevel::TRACE: \
                    GameCore::ThreadingProxy::enqueueLogTrace(oss.str().c_str(), category); \
                    break; \
                case Gnosis::LogLevel::DBG: \
                    GameCore::ThreadingProxy::enqueueLogDebug(oss.str().c_str(), category); \
                    break; \
                case Gnosis::LogLevel::INFO: \
                    GameCore::ThreadingProxy::enqueueLogInfo(oss.str().c_str(), category); \
                    break; \
                case Gnosis::LogLevel::WARN: \
                    GameCore::ThreadingProxy::enqueueLogWarn(oss.str().c_str(), category); \
                    break; \
                case Gnosis::LogLevel::ERROR: \
                    GameCore::ThreadingProxy::enqueueLogError(oss.str().c_str(), category); \
                    break; \
                case Gnosis::LogLevel::FATAL: \
                    GameCore::ThreadingProxy::enqueueLogFatal(oss.str().c_str(), category); \
                    break; \
            } \
        } while(0)
#else
    // Default category macro for non-iOS platforms  
    #define GN_LOG_CATEGORY(level, category, msg) \
        if (Gnosis::GNLog::GetInstance().ShouldLog(level)) { \
            std::ostringstream oss; \
            oss << msg; \
            Gnosis::GNLog::GetInstance().Log(level, oss.str(), category, __FILE__, __LINE__, __FUNCTION__); \
        }
#endif

#define GN_LOG_RENDER(msg) GN_LOG_CATEGORY(Gnosis::LogLevel::DBG, "RENDER", msg)
#define GN_LOG_AUDIO(msg) GN_LOG_CATEGORY(Gnosis::LogLevel::DBG, "AUDIO", msg)
#define GN_LOG_INPUT(msg) GN_LOG_CATEGORY(Gnosis::LogLevel::DBG, "INPUT", msg)
#define GN_LOG_PHYSICS(msg) GN_LOG_CATEGORY(Gnosis::LogLevel::DBG, "PHYSICS", msg)
#define GN_LOG_NETWORK(msg) GN_LOG_CATEGORY(Gnosis::LogLevel::DBG, "NETWORK", msg)
#define GN_LOG_MEMORY(msg) GN_LOG_CATEGORY(Gnosis::LogLevel::DBG, "MEMORY", msg)