/**
 * @file ConsoleLogHandler.h
 * @brief Desktop console log handler for stdout/stderr output
 * 
 * This handler outputs log messages to the console using standard C++ streams.
 * It provides colored output on supported terminals and formats messages nicely.
 */

#pragma once

#include "../../Engine/Core/GNLog.h"
#include <iostream>
#include <fstream>
#include <string>

namespace Gnosis {

    /**
     * @class ConsoleLogHandler
     * @brief Desktop console log handler that outputs to stdout/stderr
     */
    class ConsoleLogHandler : public ILogHandler {
    private:
        LogLevel m_logLevel;
        bool m_useColors;
        bool m_logToFile;
        std::ofstream m_logFile;
        std::string m_logFilePath;
        
        /**
         * @brief Get ANSI color code for log level
         * @param level Log level
         * @return ANSI color code string
         */
        std::string GetColorCode(LogLevel level) const;
        
        /**
         * @brief Reset ANSI color codes
         * @return ANSI reset code string
         */
        std::string GetResetCode() const;
        
        /**
         * @brief Format a log message for output
         * @param message Log message to format
         * @return Formatted string
         */
        std::string FormatMessage(const LogMessage& message) const;
        
        /**
         * @brief Format timestamp to readable string
         * @param timestamp Timestamp in milliseconds
         * @return Formatted time string
         */
        std::string FormatTimestamp(uint64_t timestamp) const;
        
    public:
        /**
         * @brief Constructor
         * @param useColors Enable colored output (default: true)
         * @param logToFile Also log to file (default: false)
         * @param logFilePath Path for log file if enabled
         */
        ConsoleLogHandler(bool useColors = true, bool logToFile = false, 
                         const std::string& logFilePath = "game.log");
        
        /**
         * @brief Destructor
         */
        virtual ~ConsoleLogHandler() throw();
        
        // ILogHandler interface
        bool Initialize() override;
        void Shutdown() override;
        void WriteLog(const LogMessage& message) override;
        void Flush() override;
        void SetLogLevel(LogLevel level) override;
        LogLevel GetLogLevel() const override;
        
        /**
         * @brief Enable or disable colored output
         * @param enabled True to enable colors
         */
        void SetColorsEnabled(bool enabled) { m_useColors = enabled; }
        
        /**
         * @brief Check if colors are enabled
         * @return True if colors are enabled
         */
        bool AreColorsEnabled() const { return m_useColors; }
        
        /**
         * @brief Enable or disable file logging
         * @param enabled True to enable file logging
         * @param filePath Path for log file
         */
        void SetFileLogging(bool enabled, const std::string& filePath = "");
    };

} // namespace Gnosis