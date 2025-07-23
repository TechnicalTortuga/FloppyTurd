//
//  FloppyTurd-Bridging-Header.h
//  FloppyTurd (Game-Specific)
//
//  Game-specific Swift C++ Interoperability Header
//  Imports game-specific types that depend on the GameEngine
//

#ifndef FLOPPYTURD_BRIDGING_HEADER_H
#define FLOPPYTURD_BRIDGING_HEADER_H

// Import the game engine first
#import "GameEngine-Bridging-Header.h"

// Game-specific types that use the engine
#import "Game.h"
#import "Player.h"

// Game-specific levels
#import "CastleLevel.h"
#import "DesertLevel.h"
#import "SewerLevel.h"
#import "SnowLevel.h"
#import "ParkLevel.h"
#import "BossLevel.h"

// Game-specific entities
#import "Bird.h"
#import "Boss.h"
#import "Cactus.h"
#import "Coin.h"
#import "Explosion.h"
#import "Hat.h"
#import "Outhouse.h"
#import "PoopHeart.h"
#import "RatCopter.h"
#import "RatKing.h"
#import "SewerPipe.h"
#import "SnowmanEnemy.h"
#import "SpikeBall.h"
#import "ToiletPair.h"
#import "ToiletPaper.h"
#import "BrickWall.h"
#import "GoldToilets.h"

// Game-specific projectiles
#import "SnowballProjectile.h"
#import "ToiletPaperProjectile.h"

// Game-specific UI
#import "MainMenu.h"
#import "MenuButton.h"
#import "Loading.h"
#import "Credits.h"
#import "Playing.h"
#import "BossHealthBar.h"

// Game-specific systems
#import "CameraSystem.h"
#import "GameStats.h"
#import "GameSettings.h"
#import "GlobalStateManager.h"
#import "PerformanceProfiler.h"
#import "FloppyTurdInput.h"

// Swift automatically handles C++ type conversions for game-specific types

#endif /* FLOPPYTURD_BRIDGING_HEADER_H */
