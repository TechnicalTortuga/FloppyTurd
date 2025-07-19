//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#ifndef FLOPPYTURD_BRIDGING_HEADER_H
#define FLOPPYTURD_BRIDGING_HEADER_H

// Core C++ headers that Swift needs to access
// Note: PlatformTypes.h is included by other headers, so not needed here
// Note: PlatformAPI.h excluded because it depends on Swift-generated headers
// Note: Game.h excluded because it includes PlatformAPI.h which creates circular dependency
// Note: AudioManager.h excluded because it depends on ResourceManager.h -> PlatformAPI.h
#include "GlobalStateManager.h"

// Forward declarations for complex types
class GlobalStateManager;

#endif // FLOPPYTURD_BRIDGING_HEADER_H
