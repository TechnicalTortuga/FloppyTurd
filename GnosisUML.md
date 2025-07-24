{\rtf1\ansi\ansicpg1252\cocoartf2822
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fswiss\fcharset0 Helvetica;}
{\colortbl;\red255\green255\blue255;}
{\*\expandedcolortbl;;}
\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs24 \cf0 @startuml\
skinparam classAttributeIconSize 0\
skinparam monochrome true\
skinparam packageStyle rectangle\
\
' C++/Swift Interop Layer\
package "C++/Swift Interop Layer" #lightyellow \{\
  note as InteropNote\
    == **C++/Swift Interoperability Strategy** ==\
    * Use Swift 5.9+ native C++ interop (-enable-cxx-interop).\
    * Bidirectional: C++ calls Swift actors; Swift imports C++ classes.\
    * Safe bridging: Module maps, std::vector/std::string mapping.\
    * Avoid manual wrappers; use direct access with RAII/ARC harmony.\
  end note\
\}\
\
' Gnosis Core Types (C++)\
package "Gnosis Core Types (C++)" \{\
    struct GNVector2 \{ +x, y: float \}\
    struct GNColor \{ +r, g, b, a: uint8_t \}\
    struct GNRectangle \{ +x, y, width, height: float \}\
    type GNTextureHandle = uint32_t\
    type GNAudioHandle = uint32_t\
    type GNFontHandle = uint32_t\
    type HatID = uint16_t\
    type SkillID = uint16_t\
    struct Hat \{ +id: HatID; +name: string; +price: int; +texture: GNTextureHandle \}\
    struct Skill \{ +id: SkillID; +name: string; +description: string \}\
    enum InputAction \{ JUMP, SHOOT, PAUSE, MENU_CONFIRM \}\
    enum EventType \{ COLLISION, SCORE_INCREASED, PLAYER_DIED, LEVEL_COMPLETE, JUMP_PERFORMED, ENEMY_KILLED \}\
    enum GameEvent \{ LEVEL_START, DEATH, PURCHASE, SKILL_USE, AD_WATCHED \}\
    enum AssetPriority \{ CRITICAL, HIGH, MEDIUM, LOW \}\
    struct LevelConfig \{ +seed: uint32_t; +rateVarianceMin: float; +rateVarianceMax: float; +distanceVarianceMin: float; +distanceVarianceMax: float \}\
\}\
\
' Engine Core (C++)\
package "Engine Core (C++)" \{\
  class Engine \{\
    -systemManager: unique_ptr<SystemManager>\
    -stateManager: unique_ptr<StateManager>\
    -entityManager: unique_ptr<EntityManager>\
    -componentManager: unique_ptr<ComponentManager>\
    -eventManager: unique_ptr<EventManager>\
    +Engine(platform: unique_ptr<PlatformInterfaces>)\
    +init()\
    +run(dt: float)\
    +shutdown()\
  \}\
  class EventManager \{\
    +subscribe(type: EventType, listener: IEventListener*)\
    +unsubscribe(type: EventType, listener: IEventListener*)\
    +publish(event: Event)\
  \}\
  class TelemetryManager \{\
    +trackEvent(event: GameEvent, metadata: map<string, string>)\
    +recordPlayerSession(duration: float, score: int)\
    +sendBatchedTelemetry()\
    +subscribeToEvents(eventManager: EventManager*)\
  \}\
  interface IEventListener \{ +onEvent(event: Event) \}\
  struct Event \{ +type: EventType \}\
  struct CollisionEvent extends Event \{ +entityA: Entity; +entityB: Entity \}\
  struct ScoreEvent extends Event \{ +points: int \}\
  struct TelemetryEvent extends Event \{ +gameEvent: GameEvent; +metadata: map<string, string> \}\
  class SystemManager \{\
    +registerSystem(system: unique_ptr<System>)\
    +update(dt: float)\
    +render()\
  \}\
  abstract class System implements IEventListener \{\
    +update(dt: float)\
    +render()\
  \}\
  type Entity = uint32_t\
  class EntityManager \{\
    +createEntity(): Entity\
    +destroyEntity(entity: Entity)\
  \}\
  class ComponentManager \{\
    +addComponent(entity: Entity, component: T)\
    +getComponent<T>(entity: Entity): T&\
    +query<Ts...>(): vector<Entity>\
  \}\
  note right of ComponentManager: Archetype-based storage for sparse components and efficient queries (bitsets for signatures, pre-allocated pools per level)\
  class StateManager \{\
    +pushState(state: unique_ptr<GameState>)\
    +popState()\
    +swapState(state: unique_ptr<GameState>)\
  \}\
  abstract class GameState \{\
    +onEnter(engine: Engine*)\
    +onExit(engine: Engine*)\
    +update(dt: float)\
    +render()\
  \}\
  class LoadingState extends GameState \{\}\
  class MenuState extends GameState \{\}\
  class GameplayState extends GameState \{\}\
  class PauseState extends GameState \{\}\
  class Component \{\}\
  class Transform extends Component \{ +position: GNVector2; +velocity: GNVector2; +rotation: float \}\
  struct SpriteData extends Component \{ \
    +texture: GNTextureHandle\
    +sourceRect: GNRectangle\
    +destRect: GNRectangle\
    +origin: GNVector2\
    +rotation: float\
    +scale: GNVector2\
    +tint: GNColor\
    +flipX: bool\
    +flipY: bool\
    +isAnimated: bool\
    +frameCount: int\
    +frameIndex: int\
    +frameSize: GNVector2\
    +framesPerRow: int\
    +animationSpeed: float\
    +looping: bool\
    +playing: bool\
  \}\
  note right of SpriteData: Pure data structure for cache-friendly batching (SOA layout in Sprite2DSystem)\
  class Collider extends Component \{ +bounds: GNRectangle \}\
  class ProjectileComponent extends Component \{ +damage: int; +lifetime: float \}\
  class GNCamera2D extends Component \{ \
    +position: GNVector2\
    +target: GNVector2\
    +offset: GNVector2\
    +rotation: float\
    +zoom: float\
    +followTarget: Entity\
    +bounds: GNRectangle\
    +smoothing: float\
    +shake: float\
    +shakeTimer: float\
    +Update(dt: float)\
    +SetTarget(target: GNVector2)\
    +SetZoom(zoom: float)\
    +AddShake(intensity: float, duration: float)\
    +GetWorldToScreen(worldPos: GNVector2): GNVector2\
    +GetScreenToWorld(screenPos: GNVector2): GNVector2\
    +FollowTarget(dt: float)\
  \}\
  class Score extends Component \{ +currentScore: int; +multiplier: float \}\
  class DifficultyComponent extends Component \{ +pipeSpacing: float; +enemySpawnRate: float; +difficultyLevel: int; +adjustDifficulty(playerPerformance: PlayerStats) \}\
  class LayerComponent extends Component \{ +type: LayerType; +position: GNVector2; +texture: GNTextureHandle; +scrollSpeed: float; +elements: vector<LayerElement>; +AddElement(element: LayerElement*); +RemoveElement(element: LayerElement*); +Draw(camera: Camera) \}\
  class SkillComponent extends Component \{ +activeSkills: vector<SkillID>; +skillLevels: map<SkillID, int>; +activateSkill(skillId: SkillID): bool; +hasSkill(skillId: SkillID): bool \}\
  class EnvironmentComponent extends Component \{ +weatherEffects: vector<string>; +terrainType: string; +Update(dt: float) \}\
  note right of EnvironmentComponent: Handles world/environment specifics (e.g., snow in SnowLevel, sand in DesertLevel); integrates with LayerManager for rendering\
  enum LayerType \{ BACKGROUND, MIDGROUND, FOREGROUND, EFFECTS, POST_PROCESSING, UI \}\
  enum PowerUpType \{ COIN, HEART \}\
  enum PickupEffectType \{ HEAL, SCORE_BOOST \}\
  enum PipeType \{ VERTICAL_TOILET_PAIR, HORIZONTAL_SEWER, SINGLE_OUTHOUSE, SNOW_TOILET_PAIR, GOLD_TOILET_PAIR \}\
  enum SkillType \{ PASSIVE, ACTIVE \}\
  enum SkillID \{ COIN_MAGNET, SCORE_MULTIPLIER, DAMAGE_BOOST, RAPID_FIRE, SHIELD_BURST \}\
  enum EnemyType \{ RATCOPTER, SNOWMAN, SPIKEBALL, BOSS \}\
\
  enum HatType \{ NONE, COWBOY, PIRATE, WIZARD, CROWN, SANTA \}\
  enum ProjectileType \{ TOILET_PAPER, SNOWBALL \}\
   struct GameSaveData \{ +playerStats: PlayerStats; +unlockedLevels: vector<int>; +achievements: vector<string> \}\
  class PlayerStats \{\
    +coins: int\
    +hearts: int\
    +skillPoints: int\
    +hats: vector<Hat>\
    +skills: vector<Skill>\
    +totalJumps: uint64_t\
    +totalDeaths: uint64_t\
    +pipesCleared: uint64_t\
    +enemiesKilled: uint64_t\
    +UpdateStats(event: Event)\
    +earnSkillPoints(amount: int)\
    +spendSkillPoints(amount: int): bool\
  \}\
  Player *-- PlayerStats\
\}\
\
' Engine Systems (C++)\
package "Engine Systems (C++)" \{\
  struct DrawCommand \{ +type: DrawPrimitiveType; +texture: GNTextureHandle; +position: GNVector2; +color: GNColor \}\
  class RenderSystem extends System \{ +render() \}\
  class PhysicsSystem extends System \{ +update(dt: float) \}\
  class ProjectileSystem extends System \{ +update(dt: float) \}\
  class ScoringSystem extends System \{ +onEvent(event: Event) \}\
  class StatsTrackingSystem extends System \{ +onEvent(event: Event) \}\
  class CosmeticSystem extends System \{ +render() \}\
  class SkillSystem extends System \{ \
    +update(dt: float)\
    +activateSkill(entity: Entity, skillId: SkillID): bool\
    +processSkillEffects(entity: Entity, dt: float)\
    +applyPassiveSkills(entity: Entity)\
    +handleSkillUpgrade(skillId: SkillID, newLevel: int)\
  \}\
  class PipeSpawnSystem extends System \{ +update(dt: float) \}\
  class PlayerControlSystem extends System \{ +update(dt: float) \}\
  class AnimationSystem extends System \{ +update(dt: float) \}\
  note right of AnimationSystem: Processes animation data from SpriteData components in batch\
\
  class LevelManager \{ +loadLevel(id: int, engine: Engine*, config: LevelConfig); +unloadLevel() \}\
  class PipePool \{\
    +pipes: vector<Pipe>\
    +GetPipe(): Pipe\
    +RecyclePipe(pipe: Pipe)\
  \}\
  Level ..> PipePool : uses\
  FloppyTurdGame *-- EventManager\
  FloppyTurdGame *-- TelemetryManager\
\}\
\
' Abstract Platform Interfaces (C++)\
package "Abstract Platform Interfaces (C++)" \{\
  struct PlatformInterfaces \{ renderer: unique_ptr<Renderer>; inputHandler: unique_ptr<InputHandler>; audioHandler: unique_ptr<AudioHandler> \}\
  abstract class Renderer \{ +submitBatch(commands: vector<DrawCommand>); +beginFrame(); +endFrame(); +clearScreen(); +present() \}\
  abstract class InputHandler \{ +pollEvents(); +isActionPressed(action: InputAction): bool; +getMousePosition(): GNVector2; +isKeyPressed(key: int): bool \}\
  abstract class AudioHandler \{ +playSound(handle: GNAudioHandle); +playMusic(handle: GNAudioHandle); +stopAudio(handle: GNAudioHandle); +setVolume(volume: float); +pauseAll(); +resumeAll(); +isMusic(handle: GNAudioHandle): bool; +loopAudio(handle: GNAudioHandle, loop: bool) \}\
  note right of AudioHandler: Supports overlapping sounds; singular music (stops current before playing new); platform impls handle differentiation\
  note right of InputHandler: Separated polled events from state queries; supports mock implementations for testing\
\}\
\
' Implementations\
package "iOS (Swift)" #lightblue \{\
  class MetalRenderer extends Renderer \{ \
    +@MainActor renderFrame()\
    +@MainActor setupRenderPipeline()\
    +@MainActor createTexture(data: Data): GNTextureHandle\
    +nonisolated(unsafe) submitDrawCommands(commands: [DrawCommand])\
    +@MainActor present()\
  \}\
  class TouchInputHandler extends InputHandler \{ \
    +handleTouch(event: UITouch)\
    +@MainActor processTouchEvents()\
    +nonisolated(unsafe) mapTouchToAction(touch: UITouch): InputAction\
    +@MainActor updateGestureRecognizers()\
  \}\
  class AVAudioHandler extends AudioHandler \{ \
    +nonisolated(unsafe) playBackgroundMusic()\
    +@MainActor configureAudioSession()\
    +nonisolated(unsafe) loadAudioClip(path: String): GNAudioHandle\
    +@MainActor setMasterVolume(volume: Float)\
  \}\
  class iOSUIManager \{ +DrawUIElement(rect: GNRectangle); +HandleTouchInput(event: UITouch) \}\
  note right of iOSUIManager: Mobile-specific UI layout (e.g., touch-optimized buttons, portrait/landscape handling)\
\}\
package "Desktop (C++)" \{\
  class RaylibRenderer extends Renderer \{ \
    +DrawTexture(tex: GNTextureHandle, pos: GNVector2)\
    +BeginDrawing()\
    +EndDrawing()\
    +ClearBackground(color: GNColor)\
  \}\
  class RaylibInputHandler extends InputHandler \{ \
    +IsKeyDown(key: int): bool\
    +GetMousePosition(): GNVector2\
    +IsMouseButtonPressed(button: int): bool\
  \}\
  class RaylibAudioHandler extends AudioHandler \{ \
    +PlaySound(sound: GNAudioHandle)\
    +LoadSound(fileName: string): GNAudioHandle\
    +SetSoundVolume(sound: GNAudioHandle, volume: float)\
  \}\
  class RaylibUIManager \{ +DrawUIElement(rect: GNRectangle); +HandleMouseInput(pos: GNVector2) \}\
  note right of RaylibUIManager: Desktop-specific UI layout (e.g., mouse-hover effects, keyboard shortcuts)\
\}\
\
' Floppy Turd Specific (C++)\
package "Floppy Turd (Game)" #FFEBDC \{\
  class FloppyTurdGame \{ -engine: unique_ptr<Engine>; +init(platform: unique_ptr<PlatformInterfaces>); +run(dt: float); +handleEvent(event: Event) \}\
  class Player extends Entity \{ +health: int; +position: GNVector2; +state: PLAYERSTATE; +Jump(); +Shoot(); +PutTheHurtOn(damage: int); +Update(dt: float); +Render() \}\
  enum PLAYERSTATE \{ IDLE, JUMPING, SHOOTING, HURT, DEAD \}\
  class Boss extends Entity \{ +health: int; +state: State; +TakeDamage(damage: int); +Update(dt: float); +Attack() \}\
  enum State \{ IDLE, WALKING, PREPARING_ATTACK, ATTACKING, HURT, DEATH \}\
  class Enemy extends Entity \{ +Update(dt: float); +TakeDamage(); +Render() \}\
  class Bird extends Enemy \{ +FlyPattern() \}\
  class RatCopter extends Enemy \{ +Hover() \}\
  class SnowmanEnemy extends Enemy \{ +ThrowSnowball() \}\
  class RatKing extends Boss \{ +SummonMinions() \}\
  class ToiletPaper extends Enemy \{ +RollAttack() \}\
  class Projectile extends Entity \{ +damage: int; +Update(dt: float); +OnCollision() \}\
  class ToiletPaperProjectile extends Projectile \{ +Spin() \}\
  class SnowballProjectile extends Projectile \{ +FreezeEffect() \}\
  class Pickup extends Entity \{ +Collect() \}\
  class Coin extends Pickup \{ +type: CoinType (GOLD=1, BLUE=2, RED=5); +AddToScore() \}\
  class PoopHeart extends Pickup \{ +type: PoopHeartType (SMALL, BIG, INVISIBLE); +RestoreHealth() \}\
  class Level \{ \
    +layerManager: LayerManager\
    +environmentComponent: Entity\
    +enemySpawnComponent: Entity\
    +obstacleSpawnComponent: Entity\
    +pickupSpawnComponent: Entity\
    +Update(dt: float)\
    +Render(camera: Camera)\
    +AddLayer(layer: LayerComponent*)\
    +RemoveLayer(layer: LayerComponent*)\
    +SpawnObstacles()\
    +SpawnEnemies()\
  \}\
  class ParkLevel extends Level \{  \}\
  class DesertLevel extends Level \{  \}\
  class SewerLevel extends Level \{  \}\
  class SnowLevel extends Level \{  \}\
  class CastleLevel extends Level \{  \}\
  class BossLevel extends Level \{ +InitiateBossFight() \}\
  class FloppyTurdInput \{ +Update(); +IsJumpPressed(): bool; +IsShootPressed(): bool \}\
  class Playing extends GameState \{ +Update(); +Draw(); +DrawPauseMenu() \}\
  class MainMenu extends GameState \{ \
    +HandleSelection()\
    +showSettings()\
    +showCredits()\
    +exitGame()\
    +selectedOption: int\
    +menuItems: vector<string>\
  \}\
  class GameStats \{ +totalEnemiesKilled: int; +totalLevelTries: int; +SaveStats() \}\
  // Additional scripts/classes from codebase\
  class AssetManager \{ \
    +LoadTexture(path: string): GNTextureHandle\
    +preloadLevel(levelId: int)\
    +streamTextures(priority: AssetPriority)\
    +unloadUnusedAssets()\
    +getMemoryUsage(): size_t\
    +cacheTexture(handle: GNTextureHandle)\
  \}\
\
  class EnemySpawnComponent extends Component \{\
    +enemyTypes: vector<EnemyType>\
    +spawnRate: float\
    +spawnTimer: float\
    +maxEnemies: int\
    +currentEnemyCount: int\
    +spawnPositions: vector<GNVector2>\
    +Update(dt: float)\
    +SpawnEnemy(type: EnemyType)\
    +CanSpawn(): bool\
    +SetSpawnRate(rate: float)\
    +AddSpawnPosition(position: GNVector2)\
  \}\
  note right of EnemySpawnComponent: Uses LevelConfig for variance in rates/positions via simple randomization (e.g., std::rand seeded)\
\
  class ObstacleSpawnComponent extends Component \{\
    +obstacleTypes: vector<PipeType>\
    +spawnRate: float\
    +spawnTimer: float\
    +minDistance: float\
    +maxDistance: float\
    +lastSpawnPosition: GNVector2\
    +Update(dt: float)\
    +SpawnObstacle(type: PipeType)\
    +CanSpawn(): bool\
    +SetSpawnRate(rate: float)\
    +GetNextSpawnPosition(): GNVector2\
  \}\
  note right of ObstacleSpawnComponent: Uses LevelConfig for variance in rates/distances via simple randomization\
\
  class PickupSpawnComponent extends Component \{\
    +pickupTypes: vector<PowerUpType>\
    +spawnRate: float\
    +spawnTimer: float\
    +spawnChance: float\
    +spawnPositions: vector<GNVector2>\
    +Update(dt: float)\
    +SpawnPickup(type: PowerUpType)\
    +CanSpawn(): bool\
    +SetSpawnRate(rate: float)\
    +SetSpawnChance(chance: float)\
  \}\
  note right of PickupSpawnComponent: Uses LevelConfig for variance in rates/chances via simple randomization\
\
  class BossHealthBar \{ +UpdateHealth(health: int) \}\
  class Explosion \{ +Animate() \}\
  class Hat \{  \}\
  class Skill \{\
    +id: SkillID\
    +name: string\
    +description: string\
    +skillType: SkillType\
    +isUnlocked: bool\
    +level: int\
    +maxLevel: int\
    +Activate(player: Player): bool\
    +Upgrade(): bool\
    +GetEffectStrength(): float\
  \}\
  class SnowOverlay \{ +UpdateParticles(); +density: float; +windSpeed: GNVector2 \}\
  class BrickWall extends Obstacle \{ +Break(); +getDamage(): int; +durability: int \}\
  class Cactus extends Obstacle \{ +Prick(); +spikeDamage: int; +isDeadly: bool \}\
  class SpikeBall extends Obstacle \{ +Roll(); +rollSpeed: float; +spikeDamage: int \}\
  \
  ' Unified Pipe System\
  class Pipe extends Obstacle \{\
    +pipeType: PipeType\
    +CheckCollision(player: Player): bool\
    +getGapSize(): float\
    +setPosition(x: float, y: float)\
    +gapHeight: float\
    +width: float\
    +height: float\
  \}\
  note right of Pipe: Base class for all pipes; subclasses optional for type-specific behaviors (e.g., GoldToilets for scoring bonuses)\
  class GoldToilets extends Pipe \{ \
  \}\
  class Outhouse extends Pipe \{ \
  \}\
  class SewerPipe extends Pipe \{ \
  \}\
\
  class LayerManager \{\
    +layers: vector<LayerComponent*>\
    +AddLayer(layer: LayerComponent*, depth: float)\
    +RemoveLayer(layer: LayerComponent*)\
    +RenderAllLayers(camera: Camera)\
    +UpdateLayers(dt: float)\
    +SortLayersByDepth()\
  \}\
  class LayerComponent extends Component \{\
    +layerType: LayerType\
    +depth: float\
    +isVisible: bool\
    +parallaxFactor: GNVector2\
    +scrollSpeed: GNVector2\
    +tint: GNColor\
    +elements: vector<LayerElement>\
    +Render(camera: Camera)\
    +Update(dt: float)\
    +SetDepth(depth: float)\
    +SetVisible(visible: bool)\
    +SetParallaxFactor(factor: GNVector2)\
    +AddElement(element: LayerElement*)\
    +RemoveElement(element: LayerElement*)\
    +Draw(camera: Camera)\
  \}\
  class BackgroundLayer extends LayerComponent \{ \}\
  class MidgroundLayer extends LayerComponent \{ \}\
  class ForegroundLayer extends LayerComponent \{ \}\
  class EffectsLayer extends LayerComponent \{ \}\
  class PostProcessingLayer extends LayerComponent \{ \}\
  class UILayer extends LayerComponent \{ \}\
  struct LayerElement \{ +texture: GNTextureHandle; +position: GNVector2; +scale: float \}\
  note right of LayerElement: Generic element for layers (e.g., textures, entities, effects); enables flexibility without type-specific methods\
  enum EnemyAIState \{ IDLE, MOVING, ATTACKING \}\
  class Credits extends GameState \{ +ScrollText(); +scrollSpeed: float; +textLines: vector<string> \}\
  class CameraSystem extends System \{ \
    +activeCamera: GNCamera2D*\
    +layerManager: LayerManager*\
    +Update(dt: float)\
    +RenderAllLayers()\
    +SetActiveCamera(camera: GNCamera2D*)\
    +SetLayerManager(manager: LayerManager*)\
  \}\
  \
  class Sprite2DSystem extends System \{\
    +Update(dt: float)\
    +Render()\
    +RenderSprite(entity: Entity, camera: GNCamera2D*)\
    +BatchRender(sprites: vector<Entity>, camera: GNCamera2D*)\
  \}\
  note right of Sprite2DSystem: Batches SpriteData for efficient rendering (single draw calls per layer)\
\
  class GlobalStateManager \{ +SaveGame(); +LoadGame(); +gameData: GameSaveData \}\
  note right of GlobalStateManager: Binary serialization with version tags and checksums; validation/clamping on load\
\
  class PerformanceProfiler \{ +LogFPS(); +getAverageFrameTime(): float; +memoryUsage: size_t \}\
  class TextureCache \{ +GetTexture(id: int): GNTextureHandle; +cacheSize: size_t; +evictOldest() \}\
  class Window \{ +Resize(width: int, height: int); +isFullscreen: bool; +aspectRatio: float \}\
  class PickUp \{ +PickupEffect(); +effectType: PickupEffectType; +duration: float \}\
\
  class FloppyTurdController \{\
    +ProcessInput()\
    +SendIOCommands(handler: InputHandler)\
    +RetrieveFromInputHandler(handler: InputHandler)\
  \}\
\
  FloppyTurdGame ..> FloppyTurdController\
   FloppyTurdController ..> InputHandler : interacts with\
Entity ..> DifficultyComponent : has\
Entity ..> LayerComponent : has\
Entity ..> SkillComponent : has\
Entity ..> ProjectileComponent : has\
Entity ..> Score : has\
Entity ..> EnvironmentComponent : has\
    EventManager ..> TelemetryEvent : publishes\
    Player ..> SkillComponent : uses\
     SkillSystem ..> SkillComponent : manages\
Obstacle <|-- Pipe\
Pipe <|-- GoldToilets\
Pipe <|-- Outhouse\
Pipe <|-- SewerPipe\
Obstacle <|-- BrickWall\
Obstacle <|-- Cactus\
Obstacle <|-- SpikeBall\
Level o--> EnvironmentComponent : has\
LayerManager ..> EnvironmentComponent : renders via\
\}\
\
' Entry Points\
package "Entry Points" \{\
  package "Desktop (C++)" \{\
    class main_cpp \{ +game_main() \}\
    class Game \{ +RunGameDesktop() \}\
    main_cpp ..> Game : calls\
    Game ..> Engine : initializes\
  \}\
  package "iOS (Swift)" #lightblue \{\
    class AppDelegateSwift \{ \
      +@main\
      +@MainActor application(_:didFinishLaunchingWithOptions:): Bool\
      +@MainActor applicationWillResignActive(_:)\
      +@MainActor applicationDidEnterBackground(_:)\
      +@MainActor applicationWillEnterForeground(_:)\
      +@MainActor applicationDidBecomeActive(_:)\
      +nonisolated(unsafe) setupCrashReporting()\
    \}\
    class GameViewControllerSwift \{ \
      +@MainActor viewDidLoad()\
      +@MainActor viewWillAppear(_:)\
      +@MainActor viewDidAppear(_:)\
      +@MainActor setupUI()\
      +@MainActor configureConstraints()\
      +nonisolated(unsafe) initializeGameAsync()\
      +@MainActor handleMemoryWarning()\
      +@MainActor orientationChanged()\
    \}\
    class GameViewSwift \{ \
      +init()\
      +@MainActor setupView()\
      +@MainActor layoutSubviews()\
      +nonisolated(unsafe) initializeEngineComponents()\
      +@MainActor setupMetalLayer()\
      +@MainActor configureDisplayLink()\
      +nonisolated(unsafe) bridgeToCppEngine()\
      +@MainActor handleTouchEvents(_:)\
    \}\
    class GameEngine \{ \
      +init()\
      +@MainActor startGameLoop()\
      +@MainActor pauseGameLoop()\
      +@MainActor resumeGameLoop()\
      +nonisolated(unsafe) updateGameState(deltaTime: Float)\
      +@MainActor renderFrame()\
      +nonisolated(unsafe) handleCppCallbacks()\
      +@MainActor setupDisplayLink()\
      +nonisolated(unsafe) bridgeInputEvents()\
    \}\
    AppDelegateSwift ..> GameViewControllerSwift : launches\
    GameViewControllerSwift ..> GameViewSwift : creates\
    GameViewSwift ..> GameEngine : initializes\
    GameEngine ..> FloppyTurdGame : bridges via interop\
    GameEngine ..> CADisplayLink : starts loop\
  \}\
\}\
\
' Relationships\
Engine o--> SystemManager\
Engine o--> StateManager\
SystemManager o--> System\
System <|-- RenderSystem\
System <|-- PhysicsSystem\
System <|-- PlayerControlSystem\
System <|-- CameraSystem\
System <|-- Sprite2DSystem\
System <|-- SkillSystem\
System <|-- ProjectileSystem\
System <|-- ScoringSystem\
System <|-- StatsTrackingSystem\
System <|-- CosmeticSystem\
System <|-- PipeSpawnSystem\
System <|-- AnimationSystem\
PlayerControlSystem ..> FloppyTurdInput : uses\
GameplayState ..> LevelManager : uses\
FloppyTurdGame ..> Engine : owns\
Player --|> Entity\
Boss --|> Entity\
Enemy --|> Entity\
Projectile --|> Entity\
Pickup --|> Entity\
Level <|-- ParkLevel\
Boss <|-- RatKing\
Enemy <|-- Bird\
Enemy <|-- RatCopter\
Enemy <|-- SnowmanEnemy\
Enemy <|-- ToiletPaper\
Projectile <|-- ToiletPaperProjectile\
Projectile <|-- SnowballProjectile\
Pickup <|-- Coin\
Pickup <|-- PoopHeart\
GameState <|-- Playing\
GameState <|-- MainMenu\
FloppyTurdGame ..> Playing : manages\
FloppyTurdGame ..> MainMenu : manages\
Level o--> LayerManager : contains\
Level o--> EnemySpawnComponent : has\
Level o--> ObstacleSpawnComponent : has\
Level o--> PickupSpawnComponent : has\
LayerManager o--> LayerComponent : manages\
LayerComponent <|-- BackgroundLayer\
LayerComponent <|-- MidgroundLayer\
LayerComponent <|-- ForegroundLayer\
LayerComponent <|-- EffectsLayer\
LayerComponent <|-- PostProcessingLayer\
LayerComponent <|-- UILayer\
CameraSystem ..> LayerManager : renders via\
CameraSystem ..> GNCamera2D : uses\
Sprite2DSystem ..> SpriteData : renders\
Sprite2DSystem ..> GNCamera2D : uses for projection\
Entity ..> SpriteData : has\
Entity ..> GNCamera2D : has\
Entity ..> Transform : has\
Entity ..> Collider : has\
EnemySpawnComponent ..> Enemy : spawns\
ObstacleSpawnComponent ..> Obstacle : spawns\
PickupSpawnComponent ..> Pickup : spawns\
Player ..> Projectile : shoots\
Player ..> Skill : uses\
Player ..> Hat : equips\
Enemy ..> Projectile : shoots\
Boss ..> Projectile : shoots\
FloppyTurdGame ..> Credits : manages\
FloppyTurdGame ..> Level : loads via LevelManager\
LevelManager ..> LevelConfig : uses\
EventManager ..> TelemetryManager : publishes to\
TelemetryManager ..> EventManager : subscribes\
GlobalStateManager ..> GameSaveData : serializes binary\
AnimationSystem ..> SpriteData : updates\
LayerComponent ..> LayerElement : contains\
@enduml}