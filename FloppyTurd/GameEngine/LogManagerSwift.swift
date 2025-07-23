import Foundation
import os.log

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
        os_log("%{public}@", log: logger, type: .default, message)
        logQueue.async {
            self.appendToFile(message)
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
