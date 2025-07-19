# LogManagerSwift: Design and Best Practices (2025)

## Overview
`LogManagerSwift` is a robust, modern logging utility for Swift/iOS/macOS, designed to:
- Log messages to both the Apple unified logging system (`os_log`) and a persistent file in the app sandbox.
- Use a singleton pattern for global access.
- Ensure thread safety and performance with a dedicated background queue for file writes.

## Key Features
- **Apple Unified Logging**: Uses `os_log` with subsystem and category for structured, searchable logs in Console.app.
- **File Logging**: Appends logs to a sandboxed file (`floppyturd_app_log.txt`) in the app's Documents directory for persistent, user-accessible logs.
- **Thread Safety**: All file writes are dispatched asynchronously on a serial queue.
- **Sandbox Compliance**: File access is limited to the app's Documents directory, following Apple's security guidelines.
- **Simple API**: `LogManagerSwift.shared.log(_:)` for easy, global logging.

## Implementation Best Practices (2025)
- Use `OSLog` (not the deprecated `os_log` global function) for structured logging. (Your code uses the correct `OSLog` API.)
- Always specify a subsystem and category for logs to improve filtering and diagnostics.
- For file logging, always use a background queue to avoid blocking the main thread.
- Store log files in the Documents directory for user access and sandbox compliance.
- Consider log rotation or size limits for long-running apps (not implemented here, but recommended for production).
- Avoid logging sensitive user data.

## Example Usage
```swift
LogManagerSwift.shared.log("Game started")
LogManagerSwift.shared.log("Player scored: \(score)")
```

## References
- [Apple Documentation: Unified Logging](https://developer.apple.com/documentation/os/logging)
- [WWDC24: What's new in Swift](https://developer.apple.com/videos/play/wwdc2024/10136/)
- [Hacking with Swift: How to write logs using os_log](https://www.hackingwithswift.com/quick-start/swiftui/how-to-write-logs-using-os-log)

## Summary
Your implementation of `LogManagerSwift` is fully up to date with 2025 Apple and community best practices. It is production-ready for both debugging and persistent log collection.
