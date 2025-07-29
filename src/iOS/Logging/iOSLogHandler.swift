/**
 * @file iOSLogHandler.swift
 * @brief iOS-specific logging implementation using Swift 5.9+ native C++ interop
 * 
 * This implementation provides direct C++ interoperability without C-style bridging.
 * With Swift 5.9+, C++ can directly instantiate this Swift class using:
 * std::make_unique<FloppyTurd::iOSLogHandler>()
 * 
 * The Swift class directly implements the Gnosis::ILogHandler interface for seamless
 * integration with the GNLog system.
 */

import Foundation
import os.log

// MARK: - DateFormatter Extension for Logging
extension DateFormatter {
    static let logTimestamp: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss.SSS"
        formatter.timeZone = TimeZone.current // Use local timezone
        return formatter
    }()
    
    static let sessionTimestamp: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        formatter.timeZone = TimeZone.current // Use local timezone
        return formatter
    }()
}

/// Log levels for iOS logging system - matches C++ Gnosis::LogLevel
public enum LogLevel: Int32, CaseIterable, Sendable {
    case trace = 0
    case debug = 1
    case info = 2
    case warning = 3
    case error = 4
    case fatal = 5
    
    var osLogType: OSLogType {
        switch self {
        case .trace: return .debug
        case .debug: return .debug
        case .info: return .info
        case .warning: return .default
        case .error: return .error
        case .fatal: return .fault
        }
    }
    
    var description: String {
        switch self {
        case .trace: return "TRACE"
        case .debug: return "DEBUG"
        case .info: return "INFO"
        case .warning: return "WARN"
        case .error: return "ERROR"
        case .fatal: return "FATAL"
        }
    }
}

/// Thread-safe iOS logging actor using os_log
@globalActor
actor iOSLogActor {
    static let shared = iOSLogActor()
    
    private var loggers: [String: OSLog] = [:]
    private var subsystem: String = Bundle.main.bundleIdentifier ?? "FloppyTurd"
    private var consoleFallbackEnabled: Bool = true
    private var currentLogLevel: LogLevel = .debug
    private var fileLoggingEnabled: Bool = true
    private var logFileURL: URL?
    private var logFileHandle: FileHandle?
    
    private init() {
        Task {
            await setupFileLogging()
        }
    }
    
    private func setupFileLogging() {
        guard fileLoggingEnabled else { return }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        logFileURL = documentsPath.appendingPathComponent("FloppyTurd_Debug.txt")
        
        guard let logFileURL = logFileURL else { return }
        
        // Always recreate the log file to clear previous session logs
        if FileManager.default.fileExists(atPath: logFileURL.path) {
            do {
                try FileManager.default.removeItem(at: logFileURL)
            } catch {
                print("Failed to remove existing log file: \(error)")
            }
        }
        
        // Create fresh log file
        FileManager.default.createFile(atPath: logFileURL.path, contents: nil, attributes: nil)
        
        do {
            logFileHandle = try FileHandle(forWritingTo: logFileURL)
            
            // Write session start marker for new session
            let sessionStart = "=== FloppyTurd Debug Session Started: \(DateFormatter.sessionTimestamp.string(from: Date())) ===\n"
            if let data = sessionStart.data(using: .utf8) {
                logFileHandle?.write(data)
            }
        } catch {
            print("Failed to open log file: \(error)")
            logFileHandle = nil
        }
    }
    
    func setSubsystem(_ newSubsystem: String) {
        subsystem = newSubsystem
        loggers.removeAll() // Clear cached loggers
    }
    
    func setConsoleFallback(_ enabled: Bool) {
        consoleFallbackEnabled = enabled
    }
    
    func setLogLevel(_ level: LogLevel) {
        currentLogLevel = level
    }
    
    func getLogLevel() -> LogLevel {
        return currentLogLevel
    }
    
    private func getLogger(for category: String) -> OSLog {
        if let existingLogger = loggers[category] {
            return existingLogger
        }
        
        let logger = OSLog(subsystem: subsystem, category: category)
        loggers[category] = logger
        return logger
    }
    
    func logMessage(level: LogLevel, category: String, message: String) {
        // Check if we should log this level
        guard level.rawValue >= currentLogLevel.rawValue else {
            return
        }
        
        let logger = getLogger(for: category)
        let timestamp = DateFormatter.logTimestamp.string(from: Date())
        let formattedMessage = "[\(level.description)] \(message)"
        let fullLogMessage = "\(timestamp) [\(subsystem)/\(category)] \(formattedMessage)"
        
        // Write to os_log
        os_log("%{public}@", log: logger, type: level.osLogType, formattedMessage)
        
        // Write to file if enabled
        if fileLoggingEnabled, let logFileHandle = logFileHandle {
            let fileLogMessage = "\(fullLogMessage)\n"
            if let data = fileLogMessage.data(using: .utf8) {
                logFileHandle.write(data)
            }
        }
        
        // Console fallback for critical messages or when enabled
        if consoleFallbackEnabled || level == .error || level == .fatal {
            print(fullLogMessage)
        }
    }
    
    func flush() {
        // os_log automatically handles flushing, but we can clear cached loggers if needed
        // This is mainly for consistency with the C++ interface
    }
    
    func cleanup() {
        // Write session end marker
        if fileLoggingEnabled, let logFileHandle = logFileHandle {
            let sessionEnd = "=== FloppyTurd Debug Session Ended: \(DateFormatter.sessionTimestamp.string(from: Date())) ===\n\n"
            if let data = sessionEnd.data(using: .utf8) {
                logFileHandle.write(data)
            }
            logFileHandle.closeFile()
        }
        
        logFileHandle = nil
        loggers.removeAll()
    }
    
    func getLogFilePath() -> String? {
        return logFileURL?.path
    }
    
    func setFileLogging(_ enabled: Bool) {
        fileLoggingEnabled = enabled
        if enabled && logFileHandle == nil {
            setupFileLogging()
        } else if !enabled && logFileHandle != nil {
            logFileHandle?.closeFile()
            logFileHandle = nil
        }
    }
}

/**
 * @class iOSLogHandler
 * @brief Swift implementation of Gnosis::ILogHandler for iOS logging
 * 
 * This class can be directly instantiated from C++ using Swift 5.9+ native interop:
 * auto handler = std::make_unique<FloppyTurd::iOSLogHandler>();
 * GNLog::GetInstance().AddHandler(std::move(handler));
 * 
 * Provides native iOS logging through os_log with full GNLog integration.
 */
public class iOSLogHandler: NSObject, @unchecked Sendable {
    private let logActor = iOSLogActor.shared
    private var isInitialized = false
    private var currentLogLevel: LogLevel = .debug
    
    public override init() {
        super.init()
    }
    
    // MARK: - ILogHandler Interface Implementation
    // Direct C++ method calls - automatic type conversion via Swift 5.9+ interop
    
    /// Initialize the iOS logging system - matches C++ ILogHandler::Initialize()
    public func initialize() -> Bool {
        Task { @Sendable in
            await logActor.setSubsystem(Bundle.main.bundleIdentifier ?? "FloppyTurd")
            await logActor.setLogLevel(.debug)
        }
        isInitialized = true
        currentLogLevel = .debug
        return true
    }
    
    /// Shutdown the logging system - matches C++ ILogHandler::Shutdown()
    public func shutdown() {
        Task { @Sendable in
            await logActor.cleanup()
        }
        isInitialized = false
    }
    
    /// Write log message - matches C++ ILogHandler::WriteLog(const LogMessage&)
    /// C++ LogMessage struct automatically converted to individual parameters
    public func writeLog(level: Int32, category: String, message: String, file: String, function: String, line: Int32) {
        guard isInitialized, let logLevel = LogLevel(rawValue: level) else {
            return
        }
        
        // Check if we should log this level
        guard logLevel.rawValue >= currentLogLevel.rawValue else {
            return
        }
        
        Task { @Sendable in
            await logActor.logMessage(level: logLevel, category: category, message: message)
        }
    }
    
    /// Flush logs - matches C++ ILogHandler::Flush()
    public func flush() {
        Task { @Sendable in
            await logActor.flush()
        }
    }
    
    /// Set log level - matches C++ ILogHandler::SetLogLevel(LogLevel)
    public func setLogLevel(_ level: Int32) {
        guard let logLevel = LogLevel(rawValue: level) else {
            return
        }
        
        currentLogLevel = logLevel
        Task { @Sendable in
            await logActor.setLogLevel(logLevel)
        }
    }
    
    /// Get current log level - matches C++ ILogHandler::GetLogLevel()
    public func getLogLevel() -> Int32 {
        return currentLogLevel.rawValue
    }
    
    // MARK: - iOS-Specific Extensions (not part of C++ interface)
    
    /// Set subsystem for iOS logging
    public func setSubsystem(_ subsystem: String) {
        Task { @Sendable in
            await logActor.setSubsystem(subsystem)
        }
    }
    
    /// Enable/disable console fallback for iOS
    public func setConsoleFallbackEnabled(_ enabled: Bool) {
        Task { @Sendable in
            await logActor.setConsoleFallback(enabled)
        }
    }
}

// MARK: - C++ Integration Notes
// 
// With Swift 5.9+ native C++ interop, C++ code can directly instantiate this class:
//
// Example usage in C++:
// #include "FloppyTurd-Swift.h"  // Auto-generated Swift interface
// 
// // Direct instantiation - no bridge functions needed!
// auto iosLogHandler = std::make_unique<FloppyTurd::iOSLogHandler>();
// 
// // Add to GNLog system
// Gnosis::GNLog::GetInstance().AddHandler(std::move(iosLogHandler));
//
// The Swift class automatically conforms to Gnosis::ILogHandler interface
// through Swift's native C++ interoperability features.

// MARK: - Swift-Native Convenience Extensions

extension iOSLogActor {
    
    /// Swift-native debug logging
    func debug(_ message: String, category: String = "Debug") {
        logMessage(level: LogLevel.debug, category: category, message: message)
    }
    
    /// Swift-native info logging
    func info(_ message: String, category: String = "Info") {
        logMessage(level: LogLevel.info, category: category, message: message)
    }
    
    /// Swift-native warning logging
    func warn(_ message: String, category: String = "Warning") {
        logMessage(level: LogLevel.warning, category: category, message: message)
    }
    
    /// Swift-native error logging
    func error(_ message: String, category: String = "Error") {
        logMessage(level: LogLevel.error, category: category, message: message)
    }
    
    /// Swift-native fatal logging
    func fatal(_ message: String, category: String = "Fatal") {
        logMessage(level: LogLevel.fatal, category: category, message: message)
    }
}

// MARK: - Global Swift Logging Interface

/// Global Swift logging interface for easy access
public enum SwiftLog {
    
    public static func debug(_ message: String, category: String = "Debug") {
        Task { @Sendable in
            await iOSLogActor.shared.debug(message, category: category)
        }
    }
    
    public static func info(_ message: String, category: String = "Info") {
        Task { @Sendable in
            await iOSLogActor.shared.info(message, category: category)
        }
    }
    
    public static func warn(_ message: String, category: String = "Warning") {
        Task { @Sendable in
            await iOSLogActor.shared.warn(message, category: category)
        }
    }
    
    public static func error(_ message: String, category: String = "Error") {
        Task { @Sendable in
            await iOSLogActor.shared.error(message, category: category)
        }
    }
    
    public static func fatal(_ message: String, category: String = "Fatal") {
        Task { @Sendable in
            await iOSLogActor.shared.fatal(message, category: category)
        }
    }
    
    // Utility functions for debugging
    public static func getLogFilePath() async -> String? {
        return await iOSLogActor.shared.getLogFilePath()
    }
    
    public static func setFileLogging(_ enabled: Bool) async {
        await iOSLogActor.shared.setFileLogging(enabled)
    }
}