//
//  FontResourceManager.swift
//  Professional Game Engine - Font Resource Management
//
//  Swift font resource manager for C++ interoperability
//

import Foundation
#if canImport(UIKit)
import UIKit
#endif

/// Simple font resource manager for C++ compatibility - non-actor for C++ interop
public final class FontResourceManager: @unchecked Sendable {
    public static let shared = FontResourceManager()
    
    private var fonts: [Int32: UIFont] = [:]
    private var nextFontId: Int32 = 1
    private let lock = NSLock()
    
    private init() {}
    
    /// Create a font and return its ID for C++ compatibility
    public func createFont(name: String, size: CGFloat) async -> Int32 {
        let font: UIFont
        
        // Try to load custom font or fallback to system font
        if let customFont = UIFont(name: name, size: size) {
            font = customFont
        } else {
            font = UIFont.systemFont(ofSize: size)
        }
        
        lock.lock()
        defer { lock.unlock() }
        
        let fontId = nextFontId
        fonts[fontId] = font
        nextFontId += 1
        
        print("[FontResourceManager] ✅ Created font '\(name)' with ID: \(fontId)")
        return fontId
    }
    
    /// Remove a font by ID
    public func removeFont(id: Int32) async {
        lock.lock()
        defer { lock.unlock() }
        
        fonts.removeValue(forKey: id)
        print("[FontResourceManager] 🗑️ Removed font with ID: \(id)")
    }
    
    /// Get a font by ID
    public func getFont(id: Int32) -> UIFont? {
        lock.lock()
        defer { lock.unlock() }
        
        return fonts[id]
    }
    
    /// Clean up all fonts
    public func cleanup() {
        lock.lock()
        defer { lock.unlock() }
        
        fonts.removeAll()
        nextFontId = 1
        print("[FontResourceManager] 🧹 Cleaned up all fonts")
    }
}
