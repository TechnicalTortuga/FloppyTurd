//
//  MathUtilsSwift.swift
//  FloppyTurd
//
//  Created by Carl the Code-Conjuring Turdsmith
//  Math utility functions for C++ interop
//

import Foundation
import simd

// MARK: - Math Utilities for C++ Interop
@_expose(Cxx)
public class MathUtilsSwift {
    
    // MARK: - Random Number Generation
    @_expose(Cxx)
    nonisolated public static func getRandomValue(_ min: Int32, _ max: Int32) -> Int32 {
        return Int32.random(in: min...max)
    }
    
    @_expose(Cxx)
    nonisolated public static func getRandomFloat(_ min: Float, _ max: Float) -> Float {
        return Float.random(in: min...max)
    }
    
    @_expose(Cxx)
    nonisolated public static func setRandomSeed(_ seed: UInt32) {
        // Swift uses a global random number generator that can't be seeded directly
        // For deterministic random numbers, we'd need a custom generator
        // For now, this is a no-op as Swift's random is cryptographically secure
    }
    
    // MARK: - Vector2 Math Functions
    @_expose(Cxx)
    nonisolated public static func vector2Length(_ v: Vector2) -> Float {
        return sqrt(v.x * v.x + v.y * v.y)
    }
    
    @_expose(Cxx)
    nonisolated public static func vector2Normalize(_ v: Vector2) -> Vector2 {
        let length = vector2Length(v)
        if length > 0 {
            return Vector2(x: v.x / length, y: v.y / length)
        }
        return Vector2(x: 0, y: 0)
    }
    
    @_expose(Cxx)
    nonisolated public static func vector2Add(_ v1: Vector2, _ v2: Vector2) -> Vector2 {
        return Vector2(x: v1.x + v2.x, y: v1.y + v2.y)
    }
    
    @_expose(Cxx)
    nonisolated public static func vector2Subtract(_ v1: Vector2, _ v2: Vector2) -> Vector2 {
        return Vector2(x: v1.x - v2.x, y: v1.y - v2.y)
    }
    
    @_expose(Cxx)
    nonisolated public static func vector2Scale(_ v: Vector2, _ scale: Float) -> Vector2 {
        return Vector2(x: v.x * scale, y: v.y * scale)
    }
    
    @_expose(Cxx)
    nonisolated public static func vector2Distance(_ v1: Vector2, _ v2: Vector2) -> Float {
        return vector2Length(vector2Subtract(v1, v2))
    }
    
    // MARK: - Additional Math Utilities
    @_expose(Cxx)
    nonisolated public static func clamp(_ value: Float, _ min: Float, _ max: Float) -> Float {
        return Swift.max(min, Swift.min(max, value))
    }
    
    @_expose(Cxx)
    nonisolated public static func lerp(_ start: Float, _ end: Float, _ amount: Float) -> Float {
        return start + (end - start) * amount
    }
    
    @_expose(Cxx)
    nonisolated public static func degreesToRadians(_ degrees: Float) -> Float {
        return degrees * Float.pi / 180.0
    }
    
    @_expose(Cxx)
    nonisolated public static func radiansToDegrees(_ radians: Float) -> Float {
        return radians * 180.0 / Float.pi
    }
}

// MARK: - Vector2 Extension for convenience
extension Vector2 {
    public var length: Float {
        return MathUtilsSwift.vector2Length(self)
    }
    
    public var normalized: Vector2 {
        return MathUtilsSwift.vector2Normalize(self)
    }
    
    public func distance(to other: Vector2) -> Float {
        return MathUtilsSwift.vector2Distance(self, other)
    }
    
    public static func + (lhs: Vector2, rhs: Vector2) -> Vector2 {
        return MathUtilsSwift.vector2Add(lhs, rhs)
    }
    
    public static func - (lhs: Vector2, rhs: Vector2) -> Vector2 {
        return MathUtilsSwift.vector2Subtract(lhs, rhs)
    }
    
    public static func * (lhs: Vector2, rhs: Float) -> Vector2 {
        return MathUtilsSwift.vector2Scale(lhs, rhs)
    }
}