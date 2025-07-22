//
//  FloppyTurd-Bridging-Header.h
//  FloppyTurd
//
//  Unified C++ bridging header for Swift interoperability
//  Consolidates all C++ types that Swift needs to access
//  Created by Carl the Code-Conjuring Turdsmith
//

#ifndef FLOPPYTURD_BRIDGING_HEADER_H
#define FLOPPYTURD_BRIDGING_HEADER_H

// Core platform types and utilities (from GameEngine-Bridging-Header.h)
#include "PlatformTypes.h"

// Core game state management
#include "GlobalStateManager.h"

// Forward declarations for engine components
// Note: PlatformAPI.h excluded because it depends on Swift-generated headers
// Note: Game.h excluded because it includes PlatformAPI.h which creates circular dependency
// Note: AudioManager.h excluded because it depends on ResourceManager.h -> PlatformAPI.h
class ResourceManager;
class AudioManager;
class InputManager;
class RenderManager;
class GlobalStateManager;

// C++ Interop Bridge Function Declaration
// This function is implemented in Swift and callable from C++
extern "C" {
    void* getCppInteropBridge();
}

// FloppyTurd namespace wrapper for C++ compatibility
namespace FloppyTurd {
    // Return the opaque pointer to the CppInteropBridge singleton
    inline void* getCppInteropBridge() {
        return ::getCppInteropBridge();
    }
}

// Engine-specific type definitions that Swift needs to know about
// These are the core engine interfaces that the Swift layer interacts with

#endif // FLOPPYTURD_BRIDGING_HEADER_H
