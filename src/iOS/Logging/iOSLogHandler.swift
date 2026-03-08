/**
 * @file iOSLogHandler.swift
 * @brief iOS-specific logging implementation using Swift 5.9+ native C++ interop
 * 
 * This implementation provides direct C++ interoperability without C-style bridging.
 * With Swift 5.9+, C++ can directly instantiate this Swift class using:
 * std::make_unique<PooperTrooper::iOSLogHandler>()
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

/// Thread-safe iOS logging actor using os_log with intelligent deduplication
@globalActor
actor iOSLogActor {
    static let shared = iOSLogActor()
    
    private var loggers: [String: OSLog] = [:]
    private var subsystem: String = Bundle.main.bundleIdentifier ?? "PooperTrooper"
    private var consoleFallbackEnabled: Bool = true
    private var currentLogLevel: LogLevel = .info
    // File logging enabled only on simulator for debugging
    #if targetEnvironment(simulator)
    private var fileLoggingEnabled: Bool = true
    #else
    private var fileLoggingEnabled: Bool = false  // Disabled on device
    #endif
    private var logFileURL: URL?
    private var logFileHandle: FileHandle?
    
    // Intelligent deduplication system
    private var logPatterns: [String: LogPattern] = [:]
    private let patternThrottleInterval: TimeInterval = 10.0 // 10 seconds between identical patterns
    private let maxPatternCacheSize: Int = 1000 // Prevent memory bloat
    
    private struct LogPattern {
        let signature: String
        var lastLogTime: Date
        var suppressedCount: Int
        var category: String
        var level: LogLevel
        
        init(signature: String, category: String, level: LogLevel) {
            self.signature = signature
            self.lastLogTime = Date()
            self.suppressedCount = 0
            self.category = category
            self.level = level
        }
    }
    
    private init() {
        Task {
            await setupFileLogging()
        }
    }
    
    private func setupFileLogging() {
        guard fileLoggingEnabled else { return }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        logFileURL = documentsPath.appendingPathComponent("PooperTrooper_Debug.txt")
        
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
            let sessionStart = "=== Pooper Trooper Debug Session Started: \(DateFormatter.sessionTimestamp.string(from: Date())) ===\n"
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
        
        // Generate pattern signature for deduplication
        let patternSignature = generatePatternSignature(message: message, category: category, level: level)
        let now = Date()
        
        // Check if this is a repetitive pattern
        if let existingPattern = logPatterns[patternSignature] {
            let timeSinceLastLog = now.timeIntervalSince(existingPattern.lastLogTime)
            
            if timeSinceLastLog < patternThrottleInterval {
                // Pattern is being throttled - increment suppressed count
                logPatterns[patternSignature]?.suppressedCount += 1
                return
            } else {
                // Throttle period has passed - log with suppression info if needed
                let suppressedCount = existingPattern.suppressedCount
                if suppressedCount > 0 {
                    // Log the suppression summary first
                    let suppressionMessage = "📊 SUPPRESSED: Previous message repeated \(suppressedCount) times in last \(Int(patternThrottleInterval))s"
                    actuallyLogMessage(level: .info, category: category, message: suppressionMessage, timestamp: now)
                }
                
                // Reset pattern and log the current message
                logPatterns[patternSignature] = LogPattern(signature: patternSignature, category: category, level: level)
                actuallyLogMessage(level: level, category: category, message: message, timestamp: now)
            }
        } else {
            // New pattern - add to cache and log normally
            logPatterns[patternSignature] = LogPattern(signature: patternSignature, category: category, level: level)
            actuallyLogMessage(level: level, category: category, message: message, timestamp: now)
            
            // Cleanup cache if it's getting too large
            if logPatterns.count > maxPatternCacheSize {
                cleanupOldPatterns()
            }
        }
    }
    
    private func generatePatternSignature(message: String, category: String, level: LogLevel) -> String {
        // Extract patterns from common repetitive logs
        var signature = message
        
        // Replace entity IDs with placeholder
        signature = signature.replacingOccurrences(of: #"entity \d+"#, with: "entity XXX", options: .regularExpression)
        
        // Replace texture handles with placeholder
        signature = signature.replacingOccurrences(of: #"texture handle \d+"#, with: "texture handle XXX", options: .regularExpression)
        
        // Replace coordinates with placeholder
        signature = signature.replacingOccurrences(of: #"position \([^)]+\)"#, with: "position (XXX, XXX)", options: .regularExpression)
        
        // Replace frame numbers with placeholder
        signature = signature.replacingOccurrences(of: #"frame \d+/\d+"#, with: "frame XXX/XXX", options: .regularExpression)
        
        // Replace texture names with category if it's texture loading
        if signature.contains("Texture not ready") {
            signature = signature.replacingOccurrences(of: #"texture '[^']*'"#, with: "texture 'XXX'", options: .regularExpression)
        }
        
        // Include category and level in signature to differentiate between similar messages in different contexts
        return "\(category):\(level.description):\(signature)"
    }
    
    private func actuallyLogMessage(level: LogLevel, category: String, message: String, timestamp: Date) {
        let logger = getLogger(for: category)
        let timestampString = DateFormatter.logTimestamp.string(from: timestamp)
        let formattedMessage = "[\(level.description)] \(message)"
        let fullLogMessage = "\(timestampString) [\(subsystem)/\(category)] \(formattedMessage)"
        
        // Write to os_log (visible in Console.app)
        os_log("%{public}@", log: logger, type: level.osLogType, formattedMessage)
        
        // ALSO write to NSLog for TestFlight/Console.app visibility
        // NSLog is more reliable for viewing in Console.app than os_log in some cases
        NSLog("%@", fullLogMessage)
        
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
    
    private func cleanupOldPatterns() {
        let now = Date()
        let cleanupThreshold = patternThrottleInterval * 2 // Remove patterns older than 2x throttle interval
        
        logPatterns = logPatterns.filter { _, pattern in
            now.timeIntervalSince(pattern.lastLogTime) < cleanupThreshold
        }
    }
    
    func flush() {
        // os_log automatically handles flushing, but we can clear cached loggers if needed
        // This is mainly for consistency with the C++ interface
    }
    
    func cleanup() {
        // Log any remaining suppression summaries before cleanup
        let now = Date()
        for (_, pattern) in logPatterns {
            if pattern.suppressedCount > 0 {
                let suppressionMessage = "📊 FINAL SUPPRESSION: '\(pattern.signature)' repeated \(pattern.suppressedCount) times (category: \(pattern.category), level: \(pattern.level.description))"
                actuallyLogMessage(level: .info, category: "LogSuppression", message: suppressionMessage, timestamp: now)
            }
        }
        
        // Write session end marker
        if fileLoggingEnabled, let logFileHandle = logFileHandle {
            let sessionEnd = "=== Pooper Trooper Debug Session Ended: \(DateFormatter.sessionTimestamp.string(from: Date())) ===\n\n"
            if let data = sessionEnd.data(using: .utf8) {
                logFileHandle.write(data)
            }
            logFileHandle.closeFile()
        }
        
        logFileHandle = nil
        loggers.removeAll()
        logPatterns.removeAll()
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
 * auto handler = std::make_unique<PooperTrooper::iOSLogHandler>();
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
            await logActor.setSubsystem(Bundle.main.bundleIdentifier ?? "PooperTrooper")
            
            // Enable DEBUG logging for both simulator and device
            // We need to see ALL logs for debugging StoreKit, Leaderboards, and Ads
            await logActor.setLogLevel(.debug)        // DEBUG and above for comprehensive logging
            #if targetEnvironment(simulator)
            await logActor.setFileLogging(true)       // Write to PooperTrooper_Debug.txt on simulator only
            #else
            await logActor.setFileLogging(false)      // Disable file logging on device
            #endif
            await logActor.setConsoleFallback(true)   // Enable print() for critical logs
        }
        isInitialized = true
        currentLogLevel = .debug  // DEBUG and above for comprehensive logging
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
// #include "PooperTrooper-Swift.h"  // Auto-generated Swift interface
// 
// // Direct instantiation - no bridge functions needed!
// auto iosLogHandler = std::make_unique<PooperTrooper::iOSLogHandler>();
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

/// Thread-safe logger cache - protected by loggerQueue
private let loggerQueue = DispatchQueue(label: "com.floppyturd.loggerCache")
// SWIFT 6: These are protected by loggerQueue.sync, so they're thread-safe
nonisolated(unsafe) private var cachedLoggers: [String: OSLog] = [:]
private let logSubsystem = Bundle.main.bundleIdentifier ?? "PooperTrooper"

/// Minimum log level - set higher for production builds
/// SWIFT 6: Using nonisolated(unsafe) since we only write at init time
#if DEBUG
nonisolated(unsafe) private var minimumLogLevel: LogLevel = .debug
#else
nonisolated(unsafe) private var minimumLogLevel: LogLevel = .info
#endif

private func getOrCreateLogger(category: String) -> OSLog {
    loggerQueue.sync {
        if let logger = cachedLoggers[category] {
            return logger
        }
        let logger = OSLog(subsystem: logSubsystem, category: category)
        cachedLoggers[category] = logger
        return logger
    }
}

/// Global Swift logging interface - SYNCHRONOUS to avoid Task overhead
/// At 60fps with frequent logging, spawning Tasks caused massive CPU overhead
public enum SwiftLog {
    
    // PERFORMANCE: Direct synchronous logging - no Task spawning!
    // os_log is already thread-safe, so we can call it directly
    
    @inline(__always)
    public static func debug(_ message: String, category: String = "Debug") {
        guard minimumLogLevel.rawValue <= LogLevel.debug.rawValue else { return }
        let logger = getOrCreateLogger(category: category)
        os_log("%{public}@", log: logger, type: .debug, message)
    }
    
    @inline(__always)
    public static func info(_ message: String, category: String = "Info") {
        guard minimumLogLevel.rawValue <= LogLevel.info.rawValue else { return }
        let logger = getOrCreateLogger(category: category)
        os_log("%{public}@", log: logger, type: .info, message)
    }
    
    @inline(__always)
    public static func warn(_ message: String, category: String = "Warning") {
        guard minimumLogLevel.rawValue <= LogLevel.warning.rawValue else { return }
        let logger = getOrCreateLogger(category: category)
        os_log("%{public}@", log: logger, type: .default, message)
    }
    
    @inline(__always)
    public static func error(_ message: String, category: String = "Error") {
        // Always log errors
        let logger = getOrCreateLogger(category: category)
        os_log("%{public}@", log: logger, type: .error, message)
    }
    
    @inline(__always)
    public static func fatal(_ message: String, category: String = "Fatal") {
        // Always log fatal errors
        let logger = getOrCreateLogger(category: category)
        os_log("%{public}@", log: logger, type: .fault, message)
    }
    
    // For production, set this to .warning or .error
    public static func setMinimumLogLevel(_ level: LogLevel) {
        minimumLogLevel = level
    }
    
    // Legacy async methods for compatibility (used by iOSLogHandler)
    public static func getLogFilePath() async -> String? {
        return await iOSLogActor.shared.getLogFilePath()
    }
    
    public static func setFileLogging(_ enabled: Bool) async {
        await iOSLogActor.shared.setFileLogging(enabled)
    }
}