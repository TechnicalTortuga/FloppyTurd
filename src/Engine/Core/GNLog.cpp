/**
 * @file GNLog.cpp
 * @brief Implementation of the platform-agnostic logging system
 */

#include "GNLog.h"
#include <chrono>
#include <iostream>
#include <algorithm>

namespace Gnosis {

    // Static member definitions
    std::unique_ptr<GNLog> GNLog::s_instance = nullptr;
    std::mutex GNLog::s_mutex;

    GNLog::GNLog() 
        : m_globalLogLevel(LogLevel::INFO)
        , m_initialized(false) {
    }

    GNLog::~GNLog() {
        if (m_initialized) {
            Shutdown();
        }
    }

    GNLog& GNLog::GetInstance() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_instance) {
            s_instance = std::unique_ptr<GNLog>(new GNLog());
        }
        return *s_instance;
    }

    void GNLog::Initialize() {
        auto& instance = GetInstance();
        std::lock_guard<std::mutex> lock(instance.m_logMutex);
        
        if (!instance.m_initialized) {
            // Initialize all handlers
            for (auto& handler : instance.m_handlers) {
                if (handler) {
                    handler->Initialize();
                }
            }
            instance.m_initialized = true;
            
            // Log initialization message
            instance.Log(LogLevel::INFO, "GNLog system initialized", "SYSTEM");
        }
    }

    void GNLog::Shutdown() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_instance) {
            std::lock_guard<std::mutex> logLock(s_instance->m_logMutex);
            
            if (s_instance->m_initialized) {
                s_instance->Log(LogLevel::INFO, "GNLog system shutting down", "SYSTEM");
                
                // Flush and shutdown all handlers
                for (auto& handler : s_instance->m_handlers) {
                    if (handler) {
                        handler->Flush();
                        handler->Shutdown();
                    }
                }
                s_instance->m_initialized = false;
            }
            
            s_instance.reset();
        }
    }

    void GNLog::AddHandler(std::unique_ptr<ILogHandler> handler) {
        if (!handler) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(m_logMutex);
        
        // Initialize the handler if the system is already initialized
        if (m_initialized) {
            handler->Initialize();
        }
        
        m_handlers.push_back(std::move(handler));
    }

    void GNLog::ClearHandlers() {
        std::lock_guard<std::mutex> lock(m_logMutex);
        
        // Shutdown all handlers before clearing
        for (auto& handler : m_handlers) {
            if (handler) {
                handler->Flush();
                handler->Shutdown();
            }
        }
        
        m_handlers.clear();
    }

    void GNLog::SetGlobalLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_globalLogLevel = level;
    }

    void GNLog::Log(LogLevel level, const std::string& message, const std::string& category,
                   const std::string& file, int line, const std::string& function) {
        
        // Early exit if log level is too low
        if (!ShouldLog(level)) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(m_logMutex);
        
        if (!m_initialized || m_handlers.empty()) {
            return;
        }
        
        // Create log message
        LogMessage logMessage(level, message, category, file, line, function);
        
        // Send to all handlers
        for (auto& handler : m_handlers) {
            if (handler && level >= handler->GetLogLevel()) {
                handler->WriteLog(logMessage);
            }
        }
    }

    bool GNLog::ShouldLog(LogLevel level) const {
        return level >= m_globalLogLevel;
    }

    void GNLog::FlushAll() {
        std::lock_guard<std::mutex> lock(m_logMutex);
        
        for (auto& handler : m_handlers) {
            if (handler) {
                handler->Flush();
            }
        }
    }

    // Utility functions
    const char* LogLevelToString(LogLevel level) {
        switch (static_cast<int>(level)) {
            case static_cast<int>(LogLevel::TRACE): return "TRACE";
            case static_cast<int>(LogLevel::DBG): return "DEBUG";
            case static_cast<int>(LogLevel::INFO):  return "INFO";
            case static_cast<int>(LogLevel::WARN):  return "WARN";
            case static_cast<int>(LogLevel::ERROR): return "ERROR";
            case static_cast<int>(LogLevel::FATAL): return "FATAL";
            default: return "UNKNOWN";
        }
    }

    LogLevel StringToLogLevel(const std::string& str) {
        std::string upperStr = str;
        std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
        
        if (upperStr == "TRACE") return LogLevel::TRACE;
        if (upperStr == "DEBUG") return LogLevel::DBG;
        if (upperStr == "INFO")  return LogLevel::INFO;
        if (upperStr == "WARN")  return LogLevel::WARN;
        if (upperStr == "ERROR") return LogLevel::ERROR;
        if (upperStr == "FATAL") return LogLevel::FATAL;
        
        return LogLevel::INFO; // Default fallback
    }

} // namespace Gnosis