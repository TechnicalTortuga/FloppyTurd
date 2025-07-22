import Foundation
import os.log

// MARK: - Log Levels (matching Raylib's TraceLogLevel)
public enum LogLevel: Int32 {
    case all = 0
    case trace = 1
    case debug = 2
    case info = 3
    case warning = 4
    case error = 5
    case fatal = 6
    case none = 7
}

// MARK: - Swift-specific Log Constants (SWLOG)
public enum SWLogLevel {
    public static let SWLOG_ALL: Int32 = 0
    public static let SWLOG_TRACE: Int32 = 1
    public static let SWLOG_DEBUG: Int32 = 2
    public static let SWLOG_INFO: Int32 = 3
    public static let SWLOG_WARNING: Int32 = 4
    public static let SWLOG_ERROR: Int32 = 5
    public static let SWLOG_FATAL: Int32 = 6
    public static let SWLOG_NONE: Int32 = 7
}

// Extension to add OSLog support to LogLevel
extension LogLevel {
    var osLogType: OSLogType {
        switch self {
        case .trace, .debug:
            return .debug
        case .info:
            return .info
        case .warning:
            return .default
        case .error:
            return .error
        case .fatal:
            return .fault
        default:
            return .default
        }
    }
    
    var prefix: String {
        switch self {
        case .trace: return "[TRACE]"
        case .debug: return "[DEBUG]"
        case .info: return "[INFO]"
        case .warning: return "[WARN]"
        case .error: return "[ERROR]"
        case .fatal: return "[FATAL]"
        default: return "[LOG]"
        }
    }
}

@MainActor
public final class LogManagerSwift {
    public static nonisolated(unsafe) let shared = {
        MainActor.assumeIsolated {
            LogManagerSwift()
        }
    }()
    private let logFileName = "floppyturd_app_log.txt"
    private let logSubsystem = "com.floppyturd.app"
    private let logCategory = "General"
    private let logger: OSLog
    private let logQueue = DispatchQueue(label: "LogManagerSwiftQueue")

    private init() {
        logger = OSLog(subsystem: logSubsystem, category: logCategory)
    }

    nonisolated public func log(_ message: String) {
        log(level: .info, message: message)
    }
    
    nonisolated public func log(level: LogLevel, message: String) {
        let formattedMessage = "\(level.prefix) \(message)"
        os_log("%{public}@", log: logger, type: level.osLogType, formattedMessage)
        logQueue.async {
            self.appendToFile(formattedMessage)
        }
    }

    nonisolated private func appendToFile(_ message: String) {
        guard let logURL = self.logFileURL() else { return }
        let entry = "[FloppyTurd] " + message + "\n"
        if let data = entry.data(using: .utf8) {
            // Use MainActor.assumeIsolated to safely access FileManager from nonisolated context
            let fileExists = MainActor.assumeIsolated {
                FileManager.default.fileExists(atPath: logURL.path)
            }
            if fileExists {
                if let fileHandle = try? FileHandle(forWritingTo: logURL) {
                    fileHandle.seekToEndOfFile()
                    fileHandle.write(data)
                    fileHandle.closeFile()
                }
            } else {
                try? data.write(to: logURL)
            }
        }
    }

    nonisolated private func logFileURL() -> URL? {
        // Use MainActor.assumeIsolated to safely access FileManager from nonisolated context
        let urls = MainActor.assumeIsolated {
            FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)
        }
        guard let documentsURL = urls.first else { return nil }
        
        // Create FloppyTurdLogs directory in app container root
        let logsDirectory = documentsURL.appendingPathComponent("FloppyTurdLogs")
        
        // Ensure the directory exists
        MainActor.assumeIsolated {
            try? FileManager.default.createDirectory(at: logsDirectory, withIntermediateDirectories: true, attributes: nil)
        }
        
        return logsDirectory.appendingPathComponent(logFileName)
    }
}

// MARK: - Global Logging Functions
// Global traceLog function for Swift files to use directly
nonisolated public func traceLog(_ logLevel: Int32, _ text: String) {
    LoggingSwift.traceLog(logLevel, text)
}

// MARK: - C++ Interop Bridge
@_expose(Cxx)
public class LoggingSwift {
    
    private static nonisolated(unsafe) var currentLogLevel: LogLevel = .info
    private static let logger = Logger(subsystem: "com.floppyturd.game", category: "GameEngine")
    
    // MARK: - Log Level Management
    @_expose(Cxx)
    public static func setTraceLogLevel(_ level: Int32) {
        if let logLevel = LogLevel(rawValue: level) {
            currentLogLevel = logLevel
        }
    }
    
    @_expose(Cxx)
    public static func getTraceLogLevel() -> Int32 {
        return currentLogLevel.rawValue
    }
    
    // MARK: - Logging Functions
    @_expose(Cxx)
    public static func traceLog(_ logLevel: Int32, _ text: String) {
        guard let level = LogLevel(rawValue: logLevel),
              level.rawValue >= currentLogLevel.rawValue else {
            return
        }
        
        // Route through LogManagerSwift for unified logging
        LogManagerSwift.shared.log(level: level, message: text)
    }
    
    // Convenience logging functions
    @_expose(Cxx)
    public static func logDebug(_ message: String) {
        traceLog(SWLogLevel.SWLOG_DEBUG, message)
    }
    
    @_expose(Cxx)
    public static func logInfo(_ message: String) {
        traceLog(SWLogLevel.SWLOG_INFO, message)
    }
    
    @_expose(Cxx)
    public static func logWarning(_ message: String) {
        traceLog(SWLogLevel.SWLOG_WARNING, message)
    }
    
    @_expose(Cxx)
    public static func logError(_ message: String) {
        traceLog(SWLogLevel.SWLOG_ERROR, message)
    }
}
