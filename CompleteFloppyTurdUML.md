# Complete Floppy Turd UML Design: Gnosis Architecture with Codebase Integration

## Overview
This UML integrates the Gnosis ECS architecture with specific Floppy Turd mechanics from the codebase, emphasizing the player as a turd (not a bird—Bird is an enemy class). It leverages robust C++/Swift interop using Apple's native features for bidirectional calls, module maps, and safe bridging. The design is platform-agnostic at the core, with abstract interfaces for iOS (Swift/Metal) and desktop (C++/Raylib).

Key integrations:
- **Player (Turd)**: Health modes, skills (e.g., Turd Shot, Coin Magnet), hats, projectiles.
- **Enemies & Bosses**: Bird, RatCopter, SnowmanEnemy, RatKing with states (IDLE, ATTACKING), ToiletPaper (enemy).
- **Levels**: ParkLevel, DesertLevel, etc., managed by LevelManager.
- **Pickups**: Coin (gold=1, blue=2, red=5), PoopHeart (small, big, invisible).
- **Projectiles**: ToiletPaperProjectile, SnowballProjectile.
- **Input**: FloppyTurdInput with touch/MNK controls.
- **UI/States**: Playing, MainMenu, PauseMenu with tabs (SYSTEM, HATS, SKILLS).
- **Stats**: GameStats for tracking jumps, deaths, etc.

## Directory Structure
```
FloppyTurd/
├── Engine/         # Core ECS (C++)
├── iOS/            # Swift implementations
├── Raylib/         # Desktop C++ implementations
└── FloppyTurd/     # Game-specific logic (C++)
```

## UML Diagram (PlantUML)

```plantuml
@startuml
skinparam classAttributeIconSize 0
skinparam monochrome true
skinparam packageStyle rectangle

' C++/Swift Interop Layer
package "C++/Swift Interop Layer" #lightyellow {
  note as InteropNote
    == **C++/Swift Interoperability Strategy** ==
    * Use Swift 5.9+ native C++ interop (-enable-cxx-interop).
    * Bidirectional: C++ calls Swift actors; Swift imports C++ classes.
    * Safe bridging: Module maps, std::vector/std::string mapping.
    * Avoid manual wrappers; use direct access with RAII/ARC harmony.
  end note
}

' Gnosis Core Types (C++)
package "Gnosis Core Types (C++)" {
    struct GNVector2 { +x, y: float }
    struct GNColor { +r, g, b, a: uint8_t }
    struct GNRectangle { +x, y, width, height: float }
    type GNTextureHandle = uint32_t
    type GNSoundHandle = uint32_t
    type GNFontHandle = uint32_t
    type HatID = uint16_t
    type SkillID = uint16_t
    struct Hat { +id: HatID; +name: string; +price: int; +texture: GNTextureHandle }
    struct Skill { +id: SkillID; +name: string; +description: string }
    enum InputAction { JUMP, SHOOT, PAUSE, MENU_CONFIRM }
    enum EventType { COLLISION, SCORE_INCREASED, PLAYER_DIED, LEVEL_COMPLETE, JUMP_PERFORMED, ENEMY_KILLED }
    enum GameEvent { LEVEL_START, DEATH, PURCHASE, SKILL_USE, AD_WATCHED }
    enum AssetPriority { CRITICAL, HIGH, MEDIUM, LOW }
    enum RewardType { COINS, HAT_UNLOCK, SKILL_UNLOCK, EXTRA_LIFE }
    struct Reward { +type: RewardType; +value: int; +unlockCondition: string }
}

' Engine Core (C++)
package "Engine Core (C++)" {
  class Engine {
    -systemManager: unique_ptr<SystemManager>
    -stateManager: unique_ptr<StateManager>
    -entityManager: unique_ptr<EntityManager>
    -componentManager: unique_ptr<ComponentManager>
    -eventManager: unique_ptr<EventManager>
    +Engine(platform: unique_ptr<PlatformInterfaces>)
    +init()
    +run(dt: float)
    +shutdown()
  }
  class EventManager {
    +subscribe(type: EventType, listener: IEventListener*)
    +unsubscribe(type: EventType, listener: IEventListener*)
    +publish(event: Event)
    +trackEvent(event: GameEvent, metadata: map<string, variant>)
    +recordPlayerSession(duration: float, score: int)
    +sendBatchedTelemetry()
  }
  interface IEventListener { +onEvent(event: Event) }
  struct Event { +type: EventType }
  struct CollisionEvent extends Event { +entityA: Entity; +entityB: Entity }
  struct ScoreEvent extends Event { +points: int }
  struct TelemetryEvent extends Event { +gameEvent: GameEvent; +metadata: map<string, variant> }
  class SystemManager {
    +registerSystem(system: unique_ptr<System>)
    +update(dt: float)
    +render()
  }
  abstract class System implements IEventListener {
    +update(dt: float)
    +render()
  }
  type Entity = uint32_t
  class EntityManager {
    +createEntity(): Entity
    +destroyEntity(entity: Entity)
  }
  class ComponentManager {
    +addComponent(entity: Entity, component: T)
    +getComponent<T>(entity: Entity): T&
  }
  class StateManager {
    +pushState(state: unique_ptr<GameState>)
    +popState()
    +swapState(state: unique_ptr<GameState>)
  }
  abstract class GameState {
    +onEnter(engine: Engine*)
    +onExit(engine: Engine*)
    +update(dt: float)
    +render()
  }
  class LoadingState extends GameState {}
  class MenuState extends GameState {}
  class GameplayState extends GameState {}
  class PauseState extends GameState {}
  class Component {}
  class Transform extends Component { +position: GNVector2; +velocity: GNVector2; +rotation: float }
  class Sprite2D extends Component { 
    +texture: GNTextureHandle
    +sourceRect: GNRectangle
    +destRect: GNRectangle
    +origin: GNVector2
    +rotation: float
    +scale: GNVector2
    +tint: GNColor
    +flipX: bool
    +flipY: bool
    +isAnimated: bool
    +frameCount: int
    +frameIndex: int
    +frameSize: GNVector2
    +framesPerRow: int
    +animationSpeed: float
    +looping: bool
    +playing: bool
    +Draw(position: GNVector2)
    +SetTexture(texture: GNTextureHandle)
    +SetSourceRect(rect: GNRectangle)
    +SetScale(scale: GNVector2)
    +SetRotation(rotation: float)
    +SetTint(color: GNColor)
    +SetAnimation(frameCount: int, frameSize: GNVector2, framesPerRow: int)
    +PlayAnimation(speed: float, loop: bool)
    +StopAnimation()
    +SetFrame(frameIndex: int)
    +UpdateAnimation(deltaTime: float)
    +GetFrameRect(): GNRectangle
  }
  class Collider extends Component { +bounds: GNRectangle }
  class ProjectileComponent extends Component { +damage: int; +lifetime: float }
  class GNCamera2D extends Component { 
    +position: GNVector2
    +target: GNVector2
    +offset: GNVector2
    +rotation: float
    +zoom: float
    +followTarget: Entity
    +bounds: GNRectangle
    +smoothing: float
    +shake: float
    +shakeTimer: float
    +Update(dt: float)
    +SetTarget(target: GNVector2)
    +SetZoom(zoom: float)
    +AddShake(intensity: float, duration: float)
    +GetWorldToScreen(worldPos: GNVector2): GNVector2
    +GetScreenToWorld(screenPos: GNVector2): GNVector2
    +FollowTarget(dt: float)
  }
  class Score extends Component { +currentScore: int; +multiplier: float }
  class DifficultyComponent extends Component { +pipeSpacing: float; +enemySpawnRate: float; +difficultyLevel: int; +adjustDifficulty(playerPerformance: PlayerStats) }
  class LayerComponent extends Component { +type: LayerType; +position: GNVector2; +texture: GNTextureHandle; +scrollSpeed: float }
  class SkillComponent extends Component { +activeSkills: vector<SkillID>; +skillLevels: map<SkillID, int>; +activateSkill(skillId: SkillID): bool; +hasSkill(skillId: SkillID): bool }
  enum LayerType { BACKGROUND, MIDGROUND, FOREGROUND, EFFECTS, POST_PROCESSING, UI }
  enum PowerUpType { COIN, HEART }
  enum PickupEffectType { HEAL, SCORE_BOOST }
  enum PipeType { VERTICAL_TOILET_PAIR, HORIZONTAL_SEWER, SINGLE_OUTHOUSE, SNOW_TOILET_PAIR, GOLD_TOILET_PAIR }
  enum SkillType { PASSIVE, ACTIVE }
  enum SkillID { COIN_MAGNET, SCORE_MULTIPLIER, DAMAGE_BOOST, RAPID_FIRE, SHIELD_BURST }
  enum EnemyType { RATCOPTER, SNOWMAN, SPIKEBALL, BOSS }

  enum HatType { NONE, COWBOY, PIRATE, WIZARD, CROWN, SANTA }
  enum ProjectileType { TOILET_PAPER, SNOWBALL }
   struct GameSaveData { +playerStats: PlayerStats; +unlockedLevels: vector<int>; +achievements: vector<string> }
  class PlayerStats {
    +coins: int
    +hearts: int
    +skillPoints: int
    +hats: vector<Hat>
    +skills: vector<Skill>
    +totalJumps: uint64_t
    +totalDeaths: uint64_t
    +pipesCleared: uint64_t
    +enemiesKilled: uint64_t
    +UpdateStats(event: Event)
    +earnSkillPoints(amount: int)
    +spendSkillPoints(amount: int): bool
  }
  Player *-- PlayerStats
}

' Engine Systems (C++)
package "Engine Systems (C++)" {
  struct DrawCommand { +type: DrawPrimitiveType; +texture: GNTextureHandle; +position: GNVector2; +color: GNColor }
  class RenderSystem extends System { +render() }
  class PhysicsSystem extends System { +update(dt: float) }
  class ProjectileSystem extends System { +update(dt: float) }
  class ScoringSystem extends System { +onEvent(event: Event) }
  class StatsTrackingSystem extends System { +onEvent(event: Event) }
  class CosmeticSystem extends System { +render() }
  class SkillSystem extends System { 
    +update(dt: float)
    +activateSkill(entity: Entity, skillId: SkillID): bool
    +processSkillEffects(entity: Entity, dt: float)
    +applyPassiveSkills(entity: Entity)
    +handleSkillUpgrade(skillId: SkillID, newLevel: int)
  }
  class PipeSpawnSystem extends System { +update(dt: float) }
  class PlayerControlSystem extends System { +update(dt: float) }

  class LevelManager { +loadLevel(id: int, engine: Engine*); +unloadLevel() }
  class PipePool {
    +pipes: vector<Pipe>
    +GetPipe(): Pipe
    +RecyclePipe(pipe: Pipe)
  }
  Level ..> PipePool : uses
  FloppyTurdGame *-- EventManager
}

' Abstract Platform Interfaces (C++)
package "Abstract Platform Interfaces (C++)" {
  struct PlatformInterfaces { renderer: unique_ptr<Renderer>; inputHandler: unique_ptr<InputHandler>; audioManager: unique_ptr<AudioManager> }
  abstract class Renderer { +submitBatch(commands: vector<DrawCommand>); +beginFrame(); +endFrame(); +clearScreen(); +present() }
  abstract class InputHandler { +isActionPressed(action: InputAction): bool; +pollEvents(); +getMousePosition(): GNVector2; +isKeyPressed(key: int): bool }
  abstract class AudioManager { +playSound(handle: GNSoundHandle); +stopSound(handle: GNSoundHandle); +setVolume(volume: float); +pauseAll(); +resumeAll() }
}

' Implementations
package "iOS (Swift)" #lightblue {
  class MetalRenderer extends Renderer { 
    +@MainActor renderFrame()
    +@MainActor setupRenderPipeline()
    +@MainActor createTexture(data: Data): GNTextureHandle
    +nonisolated(unsafe) submitDrawCommands(commands: [DrawCommand])
    +@MainActor present()
  }
  class TouchInputHandler extends InputHandler { 
    +handleTouch(event: UITouch)
    +@MainActor processTouchEvents()
    +nonisolated(unsafe) mapTouchToAction(touch: UITouch): InputAction
    +@MainActor updateGestureRecognizers()
  }
  class AVAudioManager extends AudioManager { 
    +nonisolated(unsafe) playBackgroundMusic()
    +@MainActor configureAudioSession()
    +nonisolated(unsafe) loadAudioClip(path: String): GNSoundHandle
    +@MainActor setMasterVolume(volume: Float)
  }
}
package "Desktop (C++)" {
  class RaylibRenderer extends Renderer { 
    +DrawTexture(tex: GNTextureHandle, pos: GNVector2)
    +BeginDrawing()
    +EndDrawing()
    +ClearBackground(color: GNColor)
  }
  class RaylibInputHandler extends InputHandler { 
    +IsKeyDown(key: int): bool
    +GetMousePosition(): GNVector2
    +IsMouseButtonPressed(button: int): bool
  }
  class RaylibAudioManager extends AudioManager { 
    +PlaySound(sound: GNSoundHandle)
    +LoadSound(fileName: string): GNSoundHandle
    +SetSoundVolume(sound: GNSoundHandle, volume: float)
  }
}

' Floppy Turd Specific (C++)
package "Floppy Turd (Game)" #FFEBDC {
  class FloppyTurdGame { -engine: unique_ptr<Engine>; +init(platform: unique_ptr<PlatformInterfaces>); +run(dt: float); +handleEvent(event: Event) }
  class Player extends Entity { +health: int; +position: GNVector2; +state: PLAYERSTATE; +Jump(); +Shoot(); +PutTheHurtOn(damage: int); +Update(dt: float); +Render() }
  enum PLAYERSTATE { IDLE, JUMPING, SHOOTING, HURT, DEAD }
  class Boss extends Entity { +health: int; +state: State; +TakeDamage(damage: int); +Update(dt: float); +Attack() }
  enum State { IDLE, WALKING, PREPARING_ATTACK, ATTACKING, HURT, DEATH }
  class Enemy extends Entity { +Update(dt: float); +TakeDamage(); +Render() }
  class Bird extends Enemy { +FlyPattern() }
  class RatCopter extends Enemy { +Hover() }
  class SnowmanEnemy extends Enemy { +ThrowSnowball() }
  class RatKing extends Boss { +SummonMinions() }
  class ToiletPaper extends Enemy { +RollAttack() }
  class Projectile extends Entity { +damage: int; +Update(dt: float); +OnCollision() }
  class ToiletPaperProjectile extends Projectile { +Spin() }
  class SnowballProjectile extends Projectile { +FreezeEffect() }
  class Pickup extends Entity { +Collect() }
  class Coin extends Pickup { +type: CoinType (GOLD=1, BLUE=2, RED=5); +AddToScore() }
  class PoopHeart extends Pickup { +type: PoopHeartType (SMALL, BIG, INVISIBLE); +RestoreHealth() }
  class Level { 
    +layerManager: LayerManager
    +enemySpawnComponent: Entity
    +obstacleSpawnComponent: Entity
    +pickupSpawnComponent: Entity
    +Update(dt: float)
    +Render(camera: Camera)
    +AddLayer(layer: LayerComponent*)
    +RemoveLayer(layer: LayerComponent*)
    +SpawnObstacles()
    +SpawnEnemies()
  }
  class ParkLevel extends Level {  }
  class DesertLevel extends Level {  }
  class SewerLevel extends Level {  }
  class SnowLevel extends Level {  }
  class CastleLevel extends Level {  }
  class BossLevel extends Level { +InitiateBossFight() }
  class FloppyTurdInput { +Update(); +IsJumpPressed(): bool; +IsShootPressed(): bool }
  class Playing extends GameState { +Update(); +Draw(); +DrawPauseMenu() }
  class MainMenu extends GameState { 
    +HandleSelection()
    +showSettings()
    +showCredits()
    +exitGame()
    +selectedOption: int
    +menuItems: vector<string>
  }
  class GameStats { +totalEnemiesKilled: int; +totalLevelTries: int; +SaveStats() }
  // Additional scripts/classes from codebase
  class ResourceManager { 
    +LoadTexture(path: string): GNTextureHandle
    +preloadLevel(levelId: int)
    +streamTextures(priority: AssetPriority)
    +unloadUnusedAssets()
    +getMemoryUsage(): size_t
    +cacheTexture(handle: GNTextureHandle)
  }
  class UIManager { +DrawUIElement(rect: GNRectangle) }

  class EnemySpawnComponent extends Component {
    +enemyTypes: vector<EnemyType>
    +spawnRate: float
    +spawnTimer: float
    +maxEnemies: int
    +currentEnemyCount: int
    +spawnPositions: vector<GNVector2>
    +Update(dt: float)
    +SpawnEnemy(type: EnemyType)
    +CanSpawn(): bool
    +SetSpawnRate(rate: float)
    +AddSpawnPosition(position: GNVector2)
  }

  class ObstacleSpawnComponent extends Component {
    +obstacleTypes: vector<PipeType>
    +spawnRate: float
    +spawnTimer: float
    +minDistance: float
    +maxDistance: float
    +lastSpawnPosition: GNVector2
    +Update(dt: float)
    +SpawnObstacle(type: PipeType)
    +CanSpawn(): bool
    +SetSpawnRate(rate: float)
    +GetNextSpawnPosition(): GNVector2
  }

  class PickupSpawnComponent extends Component {
    +pickupTypes: vector<PowerUpType>
    +spawnRate: float
    +spawnTimer: float
    +spawnChance: float
    +spawnPositions: vector<GNVector2>
    +Update(dt: float)
    +SpawnPickup(type: PowerUpType)
    +CanSpawn(): bool
    +SetSpawnRate(rate: float)
    +SetSpawnChance(chance: float)
  }

  class BossHealthBar { +UpdateHealth(health: int) }
  class Explosion { +Animate() }
  class Hat {  }
  class Skill {
    +id: SkillID
    +name: string
    +description: string
    +skillType: SkillType
    +isUnlocked: bool
    +level: int
    +maxLevel: int
    +Activate(player: Player): bool
    +Upgrade(): bool
    +GetEffectStrength(): float
  }
  class SnowOverlay { +UpdateParticles(); +density: float; +windSpeed: GNVector2 }
  class BrickWall extends Obstacle { +Break(); +getDamage(): int; +durability: int }
  class Cactus extends Obstacle { +Prick(); +spikeDamage: int; +isDeadly: bool }
  class SpikeBall extends Obstacle { +Roll(); +rollSpeed: float; +spikeDamage: int }
  
  ' Unified Pipe System
  class Pipe extends Obstacle {
    +pipeType: PipeType
    +CheckCollision(player: Player): bool
    +getGapSize(): float
    +setPosition(x: float, y: float)
    +gapHeight: float
    +width: float
    +height: float
  }
  class GoldToilets extends Pipe { 
  }
  class Outhouse extends Pipe { 
  }
  class SewerPipe extends Pipe { 
  }

  class LayerManager {
    +layers: vector<LayerComponent*>
    +AddLayer(layer: LayerComponent*, depth: float)
    +RemoveLayer(layer: LayerComponent*)
    +RenderAllLayers(camera: Camera)
    +UpdateLayers(dt: float)
    +SortLayersByDepth()
  }
  class LayerComponent extends Component {
    +layerType: LayerType
    +depth: float
    +isVisible: bool
    +parallaxFactor: GNVector2
    +scrollSpeed: GNVector2
    +tint: GNColor
    +Render(camera: Camera)
    +Update(dt: float)
    +SetDepth(depth: float)
    +SetVisible(visible: bool)
    +SetParallaxFactor(factor: GNVector2)
  }
  class BackgroundLayer extends LayerComponent {
    +texture: GNTextureHandle
    +position: GNVector2
    +scale: float
    +repeatX: bool
    +repeatY: bool
    +Draw(camera: Camera)
    +SetTexture(texture: GNTextureHandle)
    +SetPosition(position: GNVector2)
    +SetScale(scale: float)
    +SetRepeat(x: bool, y: bool)
  }
  class MidgroundLayer extends LayerComponent {
    +entities: vector<Entity*>
    +AddEntity(entity: Entity*)
    +RemoveEntity(entity: Entity*)
    +RenderEntities(camera: Camera)
  }
  class ForegroundLayer extends LayerComponent {
    +foregroundTextures: vector<GNTextureHandle>
    +positions: vector<GNVector2>
    +AddForegroundElement(texture: GNTextureHandle, position: GNVector2)
    +RemoveForegroundElement(index: int)
    +Draw(camera: Camera)
  }
  class EffectsLayer extends LayerComponent {
    +particleSystems: vector<Entity*>
    +visualEffects: vector<Entity*>
    +AddParticleSystem(system: Entity*)
    +AddVisualEffect(effect: Entity*)
    +RenderEffects(camera: Camera)
  }
  class PostProcessingLayer extends LayerComponent {
    +shaders: vector<uint32_t>
    +renderTexture: GNTextureHandle
    +isEnabled: bool
    +ApplyPostProcessing(inputTexture: GNTextureHandle)
    +AddShader(shader: uint32_t)
    +RemoveShader(shader: uint32_t)
    +ProcessFrame(camera: Camera)
  }
  class UILayer extends LayerComponent {
    +uiElements: vector<Entity*>
    +isInteractive: bool
    +AddUIElement(element: Entity*)
    +RemoveUIElement(element: Entity*)
    +HandleInput(input: InputAction)
    +RenderUI(camera: Camera)
  }
  class Background { 
    +Scroll(speed: float)
    +updateParallax(cameraPos: GNVector2)
    +backgroundLayers: vector<Entity>
  }
  enum EnemyAIState { IDLE, MOVING, ATTACKING }
  class Credits extends GameState { +ScrollText(); +scrollSpeed: float; +textLines: vector<string> }
  class CameraSystem extends System { 
    +activeCamera: GNCamera2D*
    +layerManager: LayerManager*
    +Update(dt: float)
    +RenderAllLayers()
    +SetActiveCamera(camera: GNCamera2D*)
    +SetLayerManager(manager: LayerManager*)
  }
  
  class Sprite2DSystem extends System {
    +Update(dt: float)
    +Render()
    +RenderSprite(entity: Entity, camera: GNCamera2D*)
    +BatchRender(sprites: vector<Entity>, camera: GNCamera2D*)
  }
  class GlobalStateManager { +SaveGame(); +LoadGame(); +gameData: GameSaveData }
  class PerformanceProfiler { +LogFPS(); +getAverageFrameTime(): float; +memoryUsage: size_t }
  class SoundEffect { +Play(); +volume: float; +pitch: float; +isLooping: bool }
  class SoundManager { +MixSounds(); +masterVolume: float; +activeSounds: vector<SoundEffect> }
  class TextureCache { +GetTexture(id: int): GNTextureHandle; +cacheSize: size_t; +evictOldest() }
  class Window { +Resize(width: int, height: int); +isFullscreen: bool; +aspectRatio: float }
  class AudioClip { +Loop(); +duration: float; +sampleRate: int; +channels: int }
  class PickUp { +PickupEffect(); +effectType: PickupEffectType; +duration: float }

  class FloppyTurdController {
    +ProcessInput()
    +SendIOCommands(handler: InputHandler)
    +RetrieveFromInputHandler(handler: InputHandler)
  }
  
  class SkillTreeUI {
    +skillPoints: int
    +selectedSkill: SkillID
    +availableSkills: vector<Skill>
    +DisplaySkillTree()
    +SelectSkill(skillId: SkillID)
    +UpgradeSkill(skillId: SkillID): bool
    +HandleInput(input: InputAction)
    +RenderSkillTree()
  }
  
  class SkillHotbar {
    +activeSkills: vector<SkillID>
    +maxSlots: int
    +selectedSlot: int
    +AssignSkill(slot: int, skillID: SkillID)
    +ActivateSkill(slot: int)
    +RenderHotbar()
    +SelectSlot(slot: int)
    +GetActiveSkill(slot: int): SkillID
  }

  FloppyTurdGame ..> FloppyTurdController
   FloppyTurdController ..> InputHandler : interacts with
Entity ..> DifficultyComponent : has
Entity ..> LayerComponent : has
Entity ..> SkillComponent : has
Entity ..> ProjectileComponent : has
Entity ..> Score : has
    EventManager ..> TelemetryEvent : publishes
    Player ..> SkillComponent : uses
     SkillSystem ..> SkillComponent : manages
     FloppyTurdGame ..> SkillTreeUI : manages
Player ..> SkillHotbar : uses
SkillTreeUI ..> PlayerStats : reads
SkillHotbar ..> SkillSystem : activates
SkillTreeUI ..> Skill : displays
SkillHotbar ..> Skill : activates
Obstacle <|-- Pipe
Pipe <|-- GoldToilets
Pipe <|-- Outhouse
Pipe <|-- SewerPipe
Obstacle <|-- BrickWall
Obstacle <|-- Cactus
Obstacle <|-- SpikeBall
}

' Entry Points
package "Entry Points" {
  package "Desktop (C++)" {
    class main_cpp { +game_main() }
    class Game { +RunGameDesktop() }
    main_cpp ..> Game : calls
    Game ..> Engine : initializes
  }
  package "iOS (Swift)" #lightblue {
    class AppDelegateSwift { 
      +@main
      +@MainActor application(_:didFinishLaunchingWithOptions:): Bool
      +@MainActor applicationWillResignActive(_:)
      +@MainActor applicationDidEnterBackground(_:)
      +@MainActor applicationWillEnterForeground(_:)
      +@MainActor applicationDidBecomeActive(_:)
      +nonisolated(unsafe) setupCrashReporting()
    }
    class GameViewControllerSwift { 
      +@MainActor viewDidLoad()
      +@MainActor viewWillAppear(_:)
      +@MainActor viewDidAppear(_:)
      +@MainActor setupUI()
      +@MainActor configureConstraints()
      +nonisolated(unsafe) initializeGameAsync()
      +@MainActor handleMemoryWarning()
      +@MainActor orientationChanged()
    }
    class GameViewSwift { 
      +init()
      +@MainActor setupView()
      +@MainActor layoutSubviews()
      +nonisolated(unsafe) initializeEngineComponents()
      +@MainActor setupMetalLayer()
      +@MainActor configureDisplayLink()
      +nonisolated(unsafe) bridgeToCppEngine()
      +@MainActor handleTouchEvents(_:)
    }
    class GameEngine { 
      +init()
      +@MainActor startGameLoop()
      +@MainActor pauseGameLoop()
      +@MainActor resumeGameLoop()
      +nonisolated(unsafe) updateGameState(deltaTime: Float)
      +@MainActor renderFrame()
      +nonisolated(unsafe) handleCppCallbacks()
      +@MainActor setupDisplayLink()
      +nonisolated(unsafe) bridgeInputEvents()
    }
    AppDelegateSwift ..> GameViewControllerSwift : launches
    GameViewControllerSwift ..> GameViewSwift : creates
    GameViewSwift ..> GameEngine : initializes
    GameEngine ..> FloppyTurdGame : bridges via interop
    GameEngine ..> CADisplayLink : starts loop
  }
}

' Relationships
Engine o--> SystemManager
Engine o--> StateManager
SystemManager o--> System
System <|-- RenderSystem
System <|-- PhysicsSystem
System <|-- PlayerControlSystem
System <|-- CameraSystem
System <|-- Sprite2DSystem
System <|-- SkillSystem
System <|-- ProjectileSystem
System <|-- ScoringSystem
System <|-- StatsTrackingSystem
System <|-- CosmeticSystem
System <|-- PipeSpawnSystem
PlayerControlSystem ..> FloppyTurdInput : uses
GameplayState ..> LevelManager : uses
FloppyTurdGame ..> Engine : owns
Player --|> Entity
Boss --|> Entity
Enemy --|> Entity
Projectile --|> Entity
Pickup --|> Entity
Level <|-- ParkLevel
Boss <|-- RatKing
Enemy <|-- Bird
Enemy <|-- RatCopter
Enemy <|-- SnowmanEnemy
Enemy <|-- ToiletPaper
Projectile <|-- ToiletPaperProjectile
Projectile <|-- SnowballProjectile
Pickup <|-- Coin
Pickup <|-- PoopHeart
GameState <|-- Playing
GameState <|-- MainMenu
FloppyTurdGame ..> Playing : manages
FloppyTurdGame ..> MainMenu : manages
Level o--> LayerManager : contains
Level o--> EnemySpawnComponent : has
Level o--> ObstacleSpawnComponent : has
Level o--> PickupSpawnComponent : has
LayerManager o--> LayerComponent : manages
LayerComponent <|-- BackgroundLayer
LayerComponent <|-- MidgroundLayer
LayerComponent <|-- ForegroundLayer
LayerComponent <|-- EffectsLayer
LayerComponent <|-- PostProcessingLayer
LayerComponent <|-- UILayer
CameraSystem ..> LayerManager : renders via
CameraSystem ..> GNCamera2D : uses
Sprite2DSystem ..> Sprite2D : renders
Sprite2DSystem ..> GNCamera2D : uses for projection
MidgroundLayer ..> Entity : contains
BackgroundLayer ..> GNTextureHandle : renders
ForegroundLayer ..> GNTextureHandle : renders
EffectsLayer ..> Entity : manages particles
PostProcessingLayer ..> GNTextureHandle : processes
UILayer ..> Entity : renders UI
Entity ..> Sprite2D : has
Entity ..> GNCamera2D : has
Entity ..> Transform : has
Entity ..> Collider : has
EnemySpawnComponent ..> Enemy : spawns
ObstacleSpawnComponent ..> Obstacle : spawns
PickupSpawnComponent ..> Pickup : spawns
Player ..> Projectile : shoots
Player ..> Skill : uses
Player ..> Hat : equips
Enemy ..> Projectile : shoots
Boss ..> Projectile : shoots
FloppyTurdGame ..> Credits : manages
FloppyTurdGame ..> Level : loads via LevelManager
@enduml
```



## MVP Phase Implementation Strategy

### **Phase 1: Core Turd Mechanics (Week 1-2)**
- Basic Player (turd) with jump physics
- Simple pipe obstacles with collision
- Touch input for iOS, keyboard for desktop
- Basic scoring system
- Single level (ParkLevel)

### **Phase 2: Polish & Progression (Week 3-4)**
- Add Coin pickups and basic economy
- Implement DifficultyManager for dynamic scaling
- Add simple hat system (3-5 hats)
- Basic main menu and game over screen
- AssetStreamer for memory management

### **Phase 3: Content & Analytics (Week 5-6)**
- Add 2-3 enemy types (Bird, RatCopter)
- Implement TelemetrySystem for data collection
- Add PoopHeart health system
- Basic skill system (Turd Shot)
- Simple parallax backgrounds

### **Phase 4: iOS Store Ready (Week 7-8)**
- Complete iOS-specific optimizations
- Add Game Center integration
- Implement proper save/load system
- Polish UI/UX for App Store submission
- Performance profiling and optimization

This UML now includes three strategic improvements focused on MVP success: intelligent asset management for device compatibility, dynamic difficulty for player retention, and analytics for post-launch optimization. The phased approach ensures we can ship a polished turd that players will love!