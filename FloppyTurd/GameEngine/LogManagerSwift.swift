import Foundation
import os.log

/// Log levels for the game engine
public enum LogLevel: Int32, CaseIterable {
    case trace = 0
    case debug = 1
    case info = 2
    case warning = 3
    case error = 4
    case fatal = 5
    
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
        return documentsURL.appendingPathComponent(logFileName)
    }
}
