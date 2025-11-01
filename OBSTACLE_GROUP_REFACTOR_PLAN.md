# Obstacle Group System Refactoring Plan

## 🎯 **OBJECTIVE**
Transform the castle/snow level group wrapping system from fragile string-based logic to a robust, type-safe, manifest-driven architecture that ensures perfect synchronization of obstacles, decorations, and pickups across infinite wrapping cycles.

**Key Architectural Decision**: **LevelManager is the orchestrator** - it owns GroupManifests and coordinates ObstacleSystem/PickupSystem as services. This follows the composition pattern where LevelManager controls what spawns, when, and how groups are managed. ObstacleSystem and PickupSystem become stateless services that create entities on demand and return them to LevelManager for tracking.

---

## 🔒 **PRESERVATION GUARANTEES**
- ✅ All existing toilet positions, gaps, and spacing values preserved
- ✅ All coin positioning logic preserved (64px toilet width, gap-center calculations)
- ✅ All decoration offset values preserved
- ✅ Snow level (Level 4) and Castle level (Level 5) functionality unchanged
- ✅ Park, Sewer, Desert, and Boss levels unaffected
- ✅ No changes to rendering performance or frame rate
- ✅ **RatKing (Boss Level 6) completely unaffected** - uses BossSystem, not ObstacleSystem

---

## 🚫 **CLEAN BREAK - NO LEGACY CODE**
- ❌ NO backward compatibility fields
- ❌ NO string fields kept "temporarily"
- ✅ Delete old string-based fields immediately after adding enums
- ✅ Full migration in one session
- ✅ Test only after ALL phases complete

---

## 🏛️ **ORCHESTRATOR PATTERN - ARCHITECTURE OVERVIEW**

### **Current Problem** (Aggregation - Bad):
```
ObstacleSystem creates toilets ──► stores in m_obstacleGroups
     ↓
ObstacleSystem creates decorations ──► stores in m_obstacleGroups  
     ↓
PickupSystem creates coins ──► stores separately in m_groupCoins
     ↓
ObstacleSystem detects wrap ──► manually updates everything
     ↓
Result: Fragile, string-based, desyncs happen
```

### **New Design** (Composition - Good):
```
LevelManager (ORCHESTRATOR)
     ↓
Creates GroupManifest ──► Single source of truth
     ↓
Calls ObstacleSystem->SpawnToiletPairForGroup() ──► Returns entities
     ↓
Adds toilets to manifest
     ↓
Calls ObstacleSystem->SpawnDecorationsForGroup() ──► Returns entities
     ↓
Adds decorations to manifest
     ↓
Calls PickupSystem->SpawnCoinsForGroup() ──► Returns entities
     ↓
Adds coins to manifest
     ↓
ObstacleSystem detects wrap ──► Notifies LevelManager
     ↓
LevelManager->WrapGroup() ──► Updates ALL members atomically
     ↓
Result: Robust, type-safe, perfect sync
```

### **Roles After Refactor**:
| System | Role | Responsibilities |
|--------|------|------------------|
| **LevelManager** | Orchestrator/Owner | Owns manifests, controls lifecycle, coordinates systems |
| **ObstacleSystem** | Service | Creates obstacles/decorations, detects events, manages behavior |
| **PickupSystem** | Service | Creates coins on demand, manages collection |
| **GroupManifest** | Data | Tracks all group members (obstacles + decorations + coins) |

---

## 📊 **FILES TO BE MODIFIED**

### **Core Components**
- [x] ✅ `src/FloppyTurd/Components/GameComponents.h` - Add enums, Decoration component, GroupManifest struct

### **Systems** 
- [x] ✅ `src/FloppyTurd/Systems/LevelManager.h` - Add GroupManifest storage and orchestration methods
- [🔄] ⚠️ `src/FloppyTurd/Systems/LevelManager.cpp` - Implement SpawnGroup, WrapGroup, OnGroupOffScreen (STUBS ADDED)
- [x] ✅ `src/FloppyTurd/Systems/ObstacleSystem.h` - Add LayerManager, service method signatures
- [🔄] ⚠️ `src/FloppyTurd/Systems/ObstacleSystem.cpp` - Convert to service pattern, return entities (SIGNATURES DONE, IMPLS IN PROGRESS)
- [ ] ❌ `src/FloppyTurd/Systems/PickupSystem.h` - Add SpawnCoinsForGroup() signature
- [🔄] ⚠️ `src/FloppyTurd/Systems/PickupSystem.cpp` - Return entities, fix layer bug, use enums (LAYER FIX DONE)

### **Documentation (New Files)**
- [x] ✅ `OBSTACLE_GROUP_REFACTOR_PLAN.md` - This file
- [ ] ❌ `OBSTACLE_GROUP_REFACTOR_SUMMARY.md` - Post-completion summary

## 🏗️ **CURRENT PROGRESS (Session Checkpoint)**

**✅ COMPLETED:**
- Phase 1: Coin layer bug fixed (PickupSystem.cpp:413)
- Phase 2: ALL foundation work (enums, components, manifest structs, LevelManager storage, LayerManager)
- Phase 3 (Partial):
  - ✅ All pattern function signatures updated (accept groupId, return entities)
  - ✅ SpawnParkPattern_ToiletPair fully refactored
  - ✅ SpawnSnowPattern_ToiletPair fully refactored  
  - ✅ SpawnCastlePattern_GoldToiletPair fully refactored
  - ✅ Orchestrator method stubs added to LevelManager

**🔄 IN PROGRESS:**
- Finishing remaining pattern implementations (adding returns, removing legacy code)

**❌ REMAINING:**
- Complete sewer pattern implementations (~6 functions)
- Complete desert pattern implementations (~2 functions)
- Fully implement orchestrator methods (SpawnGroup, WrapGroup)
- Update ObstacleSystem::Update() for rightmost wrap detection
- PickupSystem refactor to return entities
- Replace all string comparisons with enums
- Refactor InitializeForLevel() to use orchestrator
- Add sewer pattern variation on wrap
- Add castle painting randomization
- Final testing and validation

---

## 📝 **PHASE 1: IMMEDIATE BUG FIXES**
**Goal**: Fix critical rendering and positioning bugs with minimal code changes.
**Estimated Time**: 30 minutes
**Risk Level**: 🟢 Low (isolated changes)

### **Task 1.1: Fix Coin Layer Bug** ✅ CRITICAL
**File**: `src/FloppyTurd/Systems/PickupSystem.cpp`
**Line**: 413
**Current Code**:
```cpp
s->layer = 3; // BUG: Coins render UNDER decorations after wrapping
```
**New Code**:
```cpp
s->layer = 5; // FIX: Coins always render ABOVE decorations (matching initial spawn)
```
**Validation**: 
- Start castle level
- Pass 50+ pipes
- Verify coins always visible on top of decorations

---

### **Task 1.2: Verify Toilet Width Consistency** ✅ DONE
**Status**: Already completed in previous session
**Verification**: All toilet width calculations use `64.0f * m_baseScale`

---

### **Task 1.3: Verify Decorations in Groups** ✅ DONE
**Status**: Already completed in previous session
**Verification**: All castle decorations added to `m_obstacleGroups[groupId]`

---

## 🏗️ **PHASE 2: FOUNDATIONAL ARCHITECTURE**
**Goal**: Introduce type-safe enums and GroupManifest system.
**Estimated Time**: 2-3 hours
**Risk Level**: 🟡 Medium (requires careful migration)

---

### **Task 2.1: Add Type-Safe Enums to GameComponents.h**

**File**: `src/FloppyTurd/Components/GameComponents.h`
**Location**: Add after existing component definitions, before struct implementations

**New Code to Add**:
```cpp
// ============================================================================
// TYPE-SAFE ENUMS (Replace string-based type checking)
// ============================================================================

enum class ObstacleType : uint8_t {
    None = 0,
    
    // Park Level (Level 1)
    TopToilet,
    BottomToilet,
    
    // Sewer Level (Level 2)
    Pipe,
    
    // Desert Level (Level 3)
    Outhouse,
    BrickWall,
    
    // Snow Level (Level 4)
    SnowToiletTop,
    SnowToiletBottom,
    Snowman,
    
    // Castle Level (Level 5)
    GoldToiletTop,
    GoldToiletBottom,
    SpikeBall,
    
    // NOTE: RatKing is NOT an obstacle - he's a boss managed by BossSystem
    
    Count // Total number of types
};

enum class DecorationType : uint8_t {
    None = 0,
    
    // Castle Level Decorations
    Curtain,
    FloorTorch,
    Chandelier,
    TorchPillar,
    
    // Castle Paintings (centerpiece - randomized on spawn/wrap)
    PaintingRabbitKnight,   // RabbitKnightPainting.png
    PaintingRatBeach,       // RatBeachPainting.png
    PaintingRiverWalk,      // RiverWalkPainting.png
    PaintingCabin,          // CabinPainting.png 
    
    Count
};

enum class PickupType : uint8_t {
    None = 0,
    
    // Coins (config strings: "GoldCoin", "BlueCoin", "RedCoin")
    GoldCoin,
    BlueCoin,
    RedCoin,
    
    // Hearts (config string: "Heart" for small, future: "HeartBig", "HeartRainbow")
    Heart,           // Standard small heart - texture: "PooHeart" (in configs as "Heart")
    HeartBig,        // Large heart - texture: "PooHeartBig" (future use, not yet in configs)
    HeartRainbow,    // Rainbow/invisible heart - texture: "PooHeartRainbow" (future Castle level every 50 pipes)
    
    Count
};

// Helper: Convert enum to string for logging (NOT for logic!)
inline const char* ToString(ObstacleType type) {
    switch(type) {
        case ObstacleType::None: return "None";
        case ObstacleType::TopToilet: return "TopToilet";
        case ObstacleType::BottomToilet: return "BottomToilet";
        case ObstacleType::Pipe: return "Pipe";
        case ObstacleType::Outhouse: return "Outhouse";
        case ObstacleType::BrickWall: return "BrickWall";
        case ObstacleType::SnowToiletTop: return "SnowToiletTop";
        case ObstacleType::SnowToiletBottom: return "SnowToiletBottom";
        case ObstacleType::Snowman: return "Snowman";
        case ObstacleType::GoldToiletTop: return "GoldToiletTop";
        case ObstacleType::GoldToiletBottom: return "GoldToiletBottom";
        case ObstacleType::SpikeBall: return "SpikeBall";
        default: return "Unknown";
    }
}

inline const char* ToString(DecorationType type) {
    switch(type) {
        case DecorationType::None: return "None";
        case DecorationType::Curtain: return "curtains"; // Texture name
        case DecorationType::FloorTorch: return "castlelevelfloortorch";
        case DecorationType::Chandelier: return "castlelevelchandelier";
        case DecorationType::TorchPillar: return "TorchPillar";
        case DecorationType::PaintingRabbitKnight: return "RabbitKnightPainting";
        case DecorationType::PaintingRatBeach: return "RatBeachPainting";
        case DecorationType::PaintingRiverWalk: return "RiverWalkPainting";
        case DecorationType::PaintingCabin: return "CabinPainting";
        default: return "Unknown";
    }
}

inline const char* ToString(PickupType type) {
    switch(type) {
        case PickupType::None: return "None";
        case PickupType::GoldCoin: return "GoldCoin";
        case PickupType::BlueCoin: return "BlueCoin";
        case PickupType::RedCoin: return "RedCoin";
        case PickupType::Heart: return "Heart"; // Config string, texture is "PooHeart"
        case PickupType::HeartBig: return "HeartBig"; // Future config string, texture "PooHeartBig"
        case PickupType::HeartRainbow: return "HeartRainbow"; // Future config, texture "PooHeartRainbow"
        default: return "Unknown";
    }
}

// Helper: Get texture name from pickup type (texture != config string for hearts!)
inline const char* GetTextureForPickup(PickupType type) {
    switch(type) {
        case PickupType::GoldCoin: return "GoldCoin";
        case PickupType::BlueCoin: return "BlueCoin";
        case PickupType::RedCoin: return "RedCoin";
        case PickupType::Heart: return "PooHeart";           // Config says "Heart", texture is "PooHeart"
        case PickupType::HeartBig: return "PooHeartBig";
        case PickupType::HeartRainbow: return "PooHeartRainbow";
        default: return "GoldCoin";
    }
}

// Helper: Check if pickup is a coin type
inline bool IsPickupCoinType(PickupType type) {
    return type == PickupType::GoldCoin || 
           type == PickupType::BlueCoin || 
           type == PickupType::RedCoin;
}

// Helper: Check if pickup is a heart type
inline bool IsPickupHeartType(PickupType type) {
    return type == PickupType::Heart ||
           type == PickupType::HeartBig ||
           type == PickupType::HeartRainbow;
}
```

**Validation**:
- Compile successfully
- No enum name collisions

---

### **Task 2.2: Update Obstacle Component to Use Enum**

**File**: `src/FloppyTurd/Components/GameComponents.h`
**Current Obstacle struct** (find around line ~400):
```cpp
struct Obstacle {
    int damage = 1;
    int health = 1;
    bool isDestructible = false;
    std::string obstacleType = "";  // ← DELETE THIS
    // ... rest of fields
};
```

**Updated Obstacle struct**:
```cpp
struct Obstacle {
    int damage = 1;
    int health = 1;
    bool isDestructible = false;
    ObstacleType type = ObstacleType::None;  // ← NEW: Type-safe enum (NO LEGACY FIELD)
    
    // ... rest of fields remain unchanged
};
```

**Validation**:
- Struct definition updated
- Old string field completely removed

---

### **Task 2.3: Add Decoration Component**

**File**: `src/FloppyTurd/Components/GameComponents.h`
**Location**: Add after Obstacle struct

**New Code to Add**:
```cpp
// ============================================================================
// DECORATION COMPONENT (Castle level decorative elements)
// ============================================================================
struct Decoration {
    DecorationType type = DecorationType::None;
    int groupId = -1;           // Which obstacle group this belongs to
    float offsetX = 0.0f;       // X offset from group leader
    float offsetY = 0.0f;       // Y offset from group leader
    int baseRenderLayer = 3;    // Base layer (can be adjusted per-decoration type)
    
    Decoration() = default;
    Decoration(DecorationType t, int gId, float ox, float oy, int layer = 3)
        : type(t), groupId(gId), offsetX(ox), offsetY(oy), baseRenderLayer(layer) {}
};
```

**Validation**:
- Compile successfully
- Component registered in ECS if needed

---

### **Task 2.4: Update Pickup Component to Use Enum**

**File**: `src/FloppyTurd/Components/GameComponents.h`
**Current Pickup struct** (find around line ~500):
```cpp
struct Pickup {
    std::string pickupType = "GoldCoin";  // ← DELETE THIS
    // ... rest of fields
};
```

**Updated Pickup struct**:
```cpp
struct Pickup {
    PickupType type = PickupType::GoldCoin;  // ← NEW: Type-safe enum (NO LEGACY FIELD)
    
    // ... rest of fields remain unchanged
};
```

**Validation**:
- Struct definition updated
- Old string field completely removed

---

### **Task 2.5: Add GroupManifest System**

**File**: `src/FloppyTurd/Components/GameComponents.h`
**Location**: Add near Group component definition

**New Code to Add**:
```cpp
// ============================================================================
// GROUP MANIFEST SYSTEM (Unified group member tracking)
// ============================================================================

// Describes a single member of an obstacle group with its relative positioning
struct GroupMemberOffset {
    Gnosis::Entity entity = 0;
    float offsetX = 0.0f;           // X offset from group leader
    float offsetY = 0.0f;           // Y offset from group leader
    int layerOffset = 0;            // Layer offset from group base layer
    bool isPickup = false;          // True if this is a coin/pickup
    bool isDecoration = false;      // True if this is a decoration
    bool isObstacle = false;        // True if this is a core obstacle
    
    GroupMemberOffset() = default;
    GroupMemberOffset(Gnosis::Entity e, float ox, float oy, int layerOff = 0)
        : entity(e), offsetX(ox), offsetY(oy), layerOffset(layerOff) {}
};

// Complete manifest describing all members of an obstacle group
struct GroupManifest {
    int groupId = -1;
    Gnosis::Entity leaderEntity = 0;    // The anchor entity (usually top toilet)
    GroupPattern pattern = GroupPattern::Unknown;
    
    // Layout parameters (single source of truth)
    float gapWidth = 0.0f;              // Distance between this group and next group's left edge
    float toiletWidth = 0.0f;           // Actual toilet sprite width (usually 64px * scale)
    int baseRenderLayer = 3;            // Base rendering layer for group
    
    // ALL group members in spawn order (determines sub-layer rendering within same layer)
    std::vector<GroupMemberOffset> allMembers;
    
    // Cached bounds for performance (calculated once, updated after wrapping)
    float leftBound = 0.0f;
    float rightBound = 0.0f;           // CRITICAL: Rightmost edge of group (for wrap detection)
    float rightmostMemberOffsetX = 0.0f; // Offset of rightmost member from leader
    bool boundsNeedRecalc = true;
    
    // Statistics (for debugging)
    int obstacleCount = 0;
    int decorationCount = 0;
    int pickupCount = 0;
    
    GroupManifest() = default;
    explicit GroupManifest(int id, Gnosis::Entity leader, GroupPattern pat, 
                          float gap, float toiletW, int layer = 3)
        : groupId(id), leaderEntity(leader), pattern(pat), 
          gapWidth(gap), toiletWidth(toiletW), baseRenderLayer(layer) {}
    
    // Add a member to the manifest
    void AddMember(Gnosis::Entity entity, float offsetX, float offsetY, 
                   int layerOffset = 0, bool isPickup = false, 
                   bool isDecoration = false, bool isObstacle = false) {
        GroupMemberOffset member(entity, offsetX, offsetY, layerOffset);
        member.isPickup = isPickup;
        member.isDecoration = isDecoration;
        member.isObstacle = isObstacle;
        allMembers.push_back(member);
        
        if (isPickup) pickupCount++;
        else if (isDecoration) decorationCount++;
        else if (isObstacle) obstacleCount++;
        
        boundsNeedRecalc = true;
    }
    
    // Recalculate group bounds based on all members
    void RecalculateBounds(Gnosis::ECSSystem* ecs) {
        if (!ecs || allMembers.empty()) return;
        
        auto* leaderTransform = ecs->GetComponent<Transform>(leaderEntity);
        if (!leaderTransform) return;
        
        float leaderX = leaderTransform->position.x;
        
        leftBound = std::numeric_limits<float>::max();
        rightBound = std::numeric_limits<float>::lowest();
        rightmostMemberOffsetX = 0.0f;
        
        for (const auto& member : allMembers) {
            auto* transform = ecs->GetComponent<Transform>(member.entity);
            auto* sprite = ecs->GetComponent<Sprite>(member.entity);
            if (!transform || !sprite) continue;
            
            float left = transform->position.x;
            float right = left + (sprite->width * transform->scale.x);
            
            leftBound = std::min(leftBound, left);
            if (right > rightBound) {
                rightBound = right;
                // Track rightmost member's offset for wrap detection
                rightmostMemberOffsetX = member.offsetX + (sprite->width * transform->scale.x);
            }
        }
        
        boundsNeedRecalc = false;
    }
};
```

**Validation**:
- Compile successfully
- No impact on runtime yet (structs just defined)

---

### **Task 2.6: Add GroupManifest Storage to LevelManager**

**File**: `src/FloppyTurd/Systems/LevelManager.h`
**Location**: In private member variables section

**Add to Private Members**:
```cpp
private:
    // NEW: Unified group manifest system (manages obstacles + decorations + pickups)
    std::unordered_map<int, GroupManifest> m_groupManifests;
```

**Add Helper Methods** (in public section):
```cpp
public:
    // Group Manifest Management (cross-system coordination)
    GroupManifest* GetGroupManifest(int groupId);
    const GroupManifest* GetGroupManifest(int groupId) const;
    bool HasGroupManifest(int groupId) const;
    void CreateGroupManifest(int groupId, Gnosis::Entity leader, GroupPattern pattern, 
                            float gapWidth, float toiletWidth, int baseLayer = 3);
    void UpdateGroupMemberPositions(int groupId, float newLeaderX);
    void ValidateGroupIntegrity(int groupId); // Debug/validation
```

**Also Update ObstacleSystem.h**:
Remove legacy `m_obstacleGroups` map entirely:
```cpp
// DELETE THIS from ObstacleSystem.h:
std::map<int, std::vector<Gnosis::Entity>> m_obstacleGroups; // ← REMOVE

// ObstacleSystem will access manifests via m_levelManager->GetGroupManifest()
```

**Validation**:
- Both headers updated
- No implementation yet (next task)

---

### **Task 2.7: Implement GroupManifest Helper Methods**

**File**: `src/FloppyTurd/Systems/LevelManager.cpp`
**Location**: Add at end of file, before closing namespace

**New Code to Add**:
```cpp
// ============================================================================
// GROUP MANIFEST MANAGEMENT (Cross-System Coordination)
// ============================================================================

GroupManifest* LevelManager::GetGroupManifest(int groupId) {
    auto it = m_groupManifests.find(groupId);
    return (it != m_groupManifests.end()) ? &it->second : nullptr;
}

const GroupManifest* LevelManager::GetGroupManifest(int groupId) const {
    auto it = m_groupManifests.find(groupId);
    return (it != m_groupManifests.end()) ? &it->second : nullptr;
}

bool LevelManager::HasGroupManifest(int groupId) const {
    return m_groupManifests.find(groupId) != m_groupManifests.end();
}

void LevelManager::CreateGroupManifest(int groupId, Gnosis::Entity leader, 
                                       GroupPattern pattern, float gapWidth, 
                                       float toiletWidth, int baseLayer) {
    GroupManifest manifest(groupId, leader, pattern, gapWidth, toiletWidth, baseLayer);
    m_groupManifests[groupId] = manifest;
    
    GN_LOG_INFO("LevelManager: Created GroupManifest for group " + std::to_string(groupId) + 
               " - Pattern: " + std::to_string(static_cast<int>(pattern)) + 
               ", Gap: " + std::to_string(gapWidth) + 
               ", ToiletWidth: " + std::to_string(toiletWidth));
}

void LevelManager::UpdateGroupMemberPositions(int groupId, float newLeaderX) {
    auto* manifest = GetGroupManifest(groupId);
    if (!manifest || !m_ecsSystem) {
        GN_LOG_WARNING("UpdateGroupMemberPositions: No manifest for group " + std::to_string(groupId));
        return;
    }
    
    // Update leader position
    auto* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
    if (!leaderTransform) {
        GN_LOG_ERROR("UpdateGroupMemberPositions: Leader entity has no Transform!");
        return;
    }
    
    float leaderY = leaderTransform->position.y; // Preserve Y
    leaderTransform->position.x = newLeaderX;
    
    // Update all member positions relative to leader
    for (auto& member : manifest->allMembers) {
        if (member.entity == manifest->leaderEntity) continue; // Leader already moved
        
        auto* transform = m_ecsSystem->GetComponent<Transform>(member.entity);
        if (!transform) continue;
        
        transform->position.x = newLeaderX + member.offsetX;
        // Y position is handled by level-specific logic (e.g., oscillating toilets in ObstacleSystem)
        
        // Ensure render layer consistency
        auto* sprite = m_ecsSystem->GetComponent<Sprite>(member.entity);
        if (sprite) {
            int correctLayer = manifest->baseRenderLayer + member.layerOffset;
            if (sprite->layer != correctLayer) {
                GN_LOG_DEBUG("UpdateGroupMemberPositions: Fixed layer for entity " + 
                           std::to_string(member.entity) + " from " + 
                           std::to_string(sprite->layer) + " to " + std::to_string(correctLayer));
                sprite->layer = correctLayer;
            }
        }
    }
    
    // Mark bounds as needing recalculation
    manifest->boundsNeedRecalc = true;
}

void LevelManager::ValidateGroupIntegrity(int groupId) {
    auto* manifest = GetGroupManifest(groupId);
    if (!manifest || !m_ecsSystem) return;
    
    GN_LOG_DEBUG("=== GROUP INTEGRITY CHECK: Group " + std::to_string(groupId) + " ===");
    GN_LOG_DEBUG("  Leader: " + std::to_string(manifest->leaderEntity));
    GN_LOG_DEBUG("  Members: " + std::to_string(manifest->allMembers.size()) + 
                " (Obstacles: " + std::to_string(manifest->obstacleCount) + 
                ", Decorations: " + std::to_string(manifest->decorationCount) + 
                ", Pickups: " + std::to_string(manifest->pickupCount) + ")");
    
    // Check all members exist and have components
    int missingEntities = 0;
    int missingTransforms = 0;
    int missingSprites = 0;
    
    for (const auto& member : manifest->allMembers) {
        if (!m_ecsSystem->EntityExists(member.entity)) {
            missingEntities++;
            continue;
        }
        if (!m_ecsSystem->HasComponent<Transform>(member.entity)) missingTransforms++;
        if (!m_ecsSystem->HasComponent<Sprite>(member.entity)) missingSprites++;
    }
    
    if (missingEntities > 0) {
        GN_LOG_ERROR("  Missing entities: " + std::to_string(missingEntities));
    }
    if (missingTransforms > 0) {
        GN_LOG_WARNING("  Missing transforms: " + std::to_string(missingTransforms));
    }
    if (missingSprites > 0) {
        GN_LOG_WARNING("  Missing sprites: " + std::to_string(missingSprites));
    }
    
    // Check relative positioning consistency
    auto* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
    if (leaderTransform) {
        int desyncCount = 0;
        for (const auto& member : manifest->allMembers) {
            if (member.entity == manifest->leaderEntity) continue;
            
            auto* transform = m_ecsSystem->GetComponent<Transform>(member.entity);
            if (!transform) continue;
            
            float expectedX = leaderTransform->position.x + member.offsetX;
            float actualX = transform->position.x;
            float error = std::abs(actualX - expectedX);
            
            if (error > 1.0f) { // More than 1 pixel off
                desyncCount++;
                if (desyncCount <= 3) { // Only log first 3
                    GN_LOG_WARNING("  DESYNC: Entity " + std::to_string(member.entity) + 
                                 " expected X=" + std::to_string(expectedX) + 
                                 " but actual X=" + std::to_string(actualX) + 
                                 " (error: " + std::to_string(error) + "px)");
                }
            }
        }
        if (desyncCount > 0) {
            GN_LOG_WARNING("  Total desynced members: " + std::to_string(desyncCount));
        } else {
            GN_LOG_DEBUG("  ✓ All members properly synchronized");
        }
    }
    
    GN_LOG_DEBUG("=== END GROUP INTEGRITY CHECK ===");
}
```

**Validation**:
- Compile successfully
- Methods available but not yet called by existing code

---

### **Task 2.8: Add LevelManager Orchestration Methods**

**File**: `src/FloppyTurd/Systems/LevelManager.h`
**Location**: Add to public methods section

**New Methods to Add**:
```cpp
public:
    // High-level orchestration (LevelManager controls spawn lifecycle)
    void SpawnGroup(int groupId, float xPosition);
    void OnGroupOffScreen(int groupId);  // Called by ObstacleSystem when wrap needed
    void WrapGroup(int groupId);         // Handles complete wrap logic
    
    // Helper for finding rightmost group during wrapping
    float GetRightmostGroupPosition() const;
```

**Implementation Notes** (will implement in Phase 3):
- `SpawnGroup()`: Orchestrates spawning of all group members (toilets, decorations, coins)
- `OnGroupOffScreen()`: Triggered by ObstacleSystem, calculates newX and calls WrapGroup()
- `WrapGroup()`: Moves all members, calls ObstacleSystem to reset state
- These methods will replace the current `ObstacleSystem::InitializeForLevel()` orchestration

**Validation**:
- Methods declared in header
- No implementation yet (Phase 3)

---

### **Task 2.9: Add Centralized Layer Management**

**File**: `src/FloppyTurd/Systems/ObstacleSystem.h`
**Location**: Add as public static utility class

**New Code to Add** (in header, before ObstacleSystem class):
```cpp
// ============================================================================
// RENDER LAYER MANAGEMENT (Centralized layer assignment)
// ============================================================================
enum class RenderLayer : int {
    Background = 0,
    BackgroundOverlay = 1,
    DecorationsBack = 2,        // Curtains, paintings
    GameObjects = 3,             // Toilets, torches, chandeliers
    Player = 4,
    PickupsAndEffects = 5,      // Coins, hearts, VFX (ALWAYS on top)
    UI = 100
};

class LayerManager {
public:
    // Get appropriate layer for obstacle type
    static int GetLayerForObstacle(ObstacleType type) {
        switch(type) {
            case ObstacleType::TopToilet:
            case ObstacleType::BottomToilet:
            case ObstacleType::GoldToiletTop:
            case ObstacleType::GoldToiletBottom:
            case ObstacleType::Pipe:
            case ObstacleType::Snowman:
            case ObstacleType::SpikeBall:
                return static_cast<int>(RenderLayer::GameObjects);
            
            case ObstacleType::Outhouse:
            case ObstacleType::BrickWall:
                return static_cast<int>(RenderLayer::GameObjects);
            
            case ObstacleType::RatKing:
            case ObstacleType::RatKingPart:
                return static_cast<int>(RenderLayer::GameObjects);
            
            default:
                return static_cast<int>(RenderLayer::GameObjects);
        }
    }
    
    // Get appropriate layer for decoration type
    static int GetLayerForDecoration(DecorationType type) {
        switch(type) {
            case DecorationType::Curtain:
            case DecorationType::Painting:
                return static_cast<int>(RenderLayer::DecorationsBack); // Behind game objects
            
            case DecorationType::FloorTorch:
            case DecorationType::Chandelier:
            case DecorationType::TorchPillar:
                return static_cast<int>(RenderLayer::GameObjects); // Same layer as toilets
            
            default:
                return static_cast<int>(RenderLayer::GameObjects);
        }
    }
    
    // Get appropriate layer for pickup type
    static int GetLayerForPickup(PickupType type) {
        // ALL pickups on layer 5 - always visible on top of obstacles
        return static_cast<int>(RenderLayer::PickupsAndEffects);
    }
};
```

**Validation**:
- Compile successfully
- Static methods available for use

---

## 🔄 **PHASE 3: ORCHESTRATOR PATTERN MIGRATION**
**Goal**: Refactor to LevelManager-as-orchestrator pattern with service-based systems.
**Estimated Time**: 3-4 hours
**Risk Level**: 🟡 Medium (significant architectural changes)

---

### **Task 3.1: Refactor ObstacleSystem Spawn Functions to Return Entities**

**Goal**: Convert spawn functions from void to returning entity vectors so LevelManager can track them.

**File**: `src/FloppyTurd/Systems/ObstacleSystem.h`

**Update Function Signatures**:
```cpp
// BEFORE:
void SpawnCastlePattern_GoldToiletPair(float x, int groupId);
void SpawnSnowPattern_ToiletPair(float x, int groupId);

// AFTER:
std::vector<Gnosis::Entity> SpawnToiletPairForGroup(int groupId, float x, GroupPattern pattern);
std::vector<Gnosis::Entity> SpawnDecorationsForGroup(int groupId, float x, GroupPattern pattern);

// New service method for state management:
void ResetGroupState(int groupId); // Reset pipeCleared, randomize Y positions, etc.
```

---

**File**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`

**Refactor Castle Toilet Spawning** (line ~2570):

```cpp
// NEW SIGNATURE - returns entities instead of void
std::vector<Gnosis::Entity> ObstacleSystem::SpawnToiletPairForGroup(
    int groupId, float x, GroupPattern pattern) 
{
    std::vector<Gnosis::Entity> entities;
    
    // Pattern-specific spawning
    if (pattern == GroupPattern::TopAndBottom && m_currentLevelId == 5) {
        // Castle level gold toilets
        const float toiletHeight = 180.0f * m_baseScale;
        const float fixedGapHeight = 700.0f;
        // ... randomize Y positions ...
        
        // Create top toilet
        Gnosis::Entity topToilet = m_ecsSystem->CreateEntity();
        // ... add all components ...
        
        // SET ENUM TYPE (no string field)
        topObstacle.type = ObstacleType::GoldToiletTop;
        topObstacle.isTopPart = true;
        
        // Use LayerManager for consistent layers
        topSprite.layer = LayerManager::GetLayerForObstacle(ObstacleType::GoldToiletTop);
        
        m_ecsSystem->AddComponent<Obstacle>(topToilet, topObstacle);
        m_ecsSystem->AddComponent<Sprite>(topToilet, topSprite);
        // ... other components ...
        
        // Create bottom toilet (same pattern)
        Gnosis::Entity bottomToilet = m_ecsSystem->CreateEntity();
        // ... setup bottom toilet ...
        bottomObstacle.type = ObstacleType::GoldToiletBottom;
        bottomSprite.layer = LayerManager::GetLayerForObstacle(ObstacleType::GoldToiletBottom);
        
        // Track in active obstacles
        m_activeObstacles.push_back(topToilet);
        m_activeObstacles.push_back(bottomToilet);
        
        // RETURN entities to LevelManager (don't create manifest here!)
        entities.push_back(topToilet);
        entities.push_back(bottomToilet);
        
        // NO m_obstacleGroups, NO manifest creation - that's LevelManager's job!
        
    } else if (pattern == GroupPattern::SnowScreenEdges && m_currentLevelId == 4) {
        // Snow level toilet pair
        // ... similar refactor ...
        entities.push_back(topToilet);
        entities.push_back(bottomToilet);
    }
    
    return entities;
}
```

**Add Decoration Spawn Function**:
```cpp
std::vector<Gnosis::Entity> ObstacleSystem::SpawnDecorationsForGroup(
    int groupId, float x, GroupPattern pattern)
{
    std::vector<Gnosis::Entity> decorations;
    
    if (m_currentLevelId == 5 && pattern == GroupPattern::TopAndBottom) {
        // Castle level decorations
        // Spawn curtain
        Gnosis::Entity curtain = SpawnCastleCurtain(x, groupId);
        decorations.push_back(curtain);
        
        // Spawn torches, chandeliers, centerpiece...
        Gnosis::Entity leftTorch = SpawnCastleFloorTorch(leftTorchX, groupId, offsetX);
        decorations.push_back(leftTorch);
        // ... etc for all decorations ...
    }
    
    return decorations; // LevelManager will add to manifest
}
```

**Add State Reset Function**:
```cpp
void ObstacleSystem::ResetGroupState(int groupId) {
    // Reset obstacle-specific state after wrapping
    for (Gnosis::Entity e : m_activeObstacles) {
        auto* group = m_ecsSystem->GetComponent<Group>(e);
        if (!group || group->id != groupId) continue;
        
        auto* obstacle = m_ecsSystem->GetComponent<Obstacle>(e);
        if (!obstacle) continue;
        
        // Reset based on type
        if (obstacle->type == ObstacleType::GoldToiletTop || 
            obstacle->type == ObstacleType::GoldToiletBottom) {
            obstacle->pipeCleared = false;
            obstacle->oscillationTimer = 0.0f;
            
            // Randomize Y position for visual variety
            if (m_currentLevelId == 5) {
                auto* transform = m_ecsSystem->GetComponent<Transform>(e);
                if (transform) {
                    float toiletHeight = 180.0f * m_baseScale;
                    float minTopY = -toiletHeight * 1.2f;
                    float maxTopY = -toiletHeight * 0.6f;
                    float randomTopY = minTopY + (maxTopY - minTopY) * ((float)rand() / RAND_MAX);
                    float fixedGapHeight = 700.0f;
                    
                    if (obstacle->type == ObstacleType::GoldToiletTop) {
                        transform->position.y = randomTopY;
                    } else {
                        transform->position.y = randomTopY + toiletHeight + fixedGapHeight;
                    }
                }
            }
        }
        else if (obstacle->type == ObstacleType::SpikeBall) {
            obstacle->pipeCleared = false;
        }
    }
}
```

**Validation**:
- ObstacleSystem no longer creates/manages manifests
- Functions return entity vectors
- ObstacleSystem is now a pure service

---

### **Task 3.2: Implement LevelManager Orchestration Methods** ⭐ **CRITICAL**

**File**: `src/FloppyTurd/Systems/LevelManager.cpp`

**Goal**: Implement the orchestrator pattern - LevelManager controls all spawning and wrapping.

**Implementation**:

```cpp
// ============================================================================
// GROUP ORCHESTRATION (LevelManager as Owner/Orchestrator)
// ============================================================================

void LevelManager::SpawnGroup(int groupId, float xPosition) {
    if (!m_obstacleSystem || !m_pickupSystem) {
        GN_LOG_ERROR("LevelManager::SpawnGroup - Systems not initialized!");
        return;
    }
    
    // Determine pattern and spacing from level config
    GroupPattern pattern = GroupPattern::TopAndBottom; // Default
    float gapWidth = 2000.0f;
    float toiletWidth = 64.0f * 8.0f; // 64px sprite * base scale
    
    if (m_currentLevelId == 4) {
        // Snow level
        pattern = GroupPattern::SnowScreenEdges;
        gapWidth = 1500.0f;
    } else if (m_currentLevelId == 5) {
        // Castle level
        pattern = GroupPattern::TopAndBottom;
        gapWidth = 2000.0f;
    }
    // ... other levels ...
    
    // STEP 1: Create manifest (LevelManager owns it)
    CreateGroupManifest(groupId, 0, pattern, gapWidth, toiletWidth, 
                       static_cast<int>(RenderLayer::GameObjects));
    
    auto* manifest = GetGroupManifest(groupId);
    if (!manifest) {
        GN_LOG_ERROR("Failed to create manifest for group " + std::to_string(groupId));
        return;
    }
    
    // STEP 2: Request toilet spawning from ObstacleSystem (service call)
    std::vector<Gnosis::Entity> toilets = 
        m_obstacleSystem->SpawnToiletPairForGroup(groupId, xPosition, pattern);
    
    if (toilets.empty()) {
        GN_LOG_ERROR("No toilets spawned for group " + std::to_string(groupId));
        return;
    }
    
    // Set leader entity (top toilet is leader)
    manifest->leaderEntity = toilets[0];
    
    // STEP 3: Add toilets to manifest
    for (size_t i = 0; i < toilets.size(); ++i) {
        auto* transform = m_ecsSystem->GetComponent<Transform>(toilets[i]);
        auto* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
        
        if (transform && leaderTransform) {
            float offsetX = transform->position.x - leaderTransform->position.x;
            float offsetY = transform->position.y - leaderTransform->position.y;
            manifest->AddMember(toilets[i], offsetX, offsetY, 0, false, false, true);
        }
    }
    
    // STEP 4: Request decorations if applicable (Castle level)
    if (m_currentLevelId == 5) {
        std::vector<Gnosis::Entity> decorations = 
            m_obstacleSystem->SpawnDecorationsForGroup(groupId, xPosition, pattern);
        
        // Add decorations to manifest
        auto* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
        for (auto decoration : decorations) {
            auto* transform = m_ecsSystem->GetComponent<Transform>(decoration);
            if (transform && leaderTransform) {
                float offsetX = transform->position.x - leaderTransform->position.x;
                float offsetY = transform->position.y - leaderTransform->position.y;
                
                // Determine layer offset based on decoration type
                auto* sprite = m_ecsSystem->GetComponent<Sprite>(decoration);
                int layerOffset = sprite ? (sprite->layer - manifest->baseRenderLayer) : 0;
                
                manifest->AddMember(decoration, offsetX, offsetY, layerOffset, 
                                   false, true, false);
            }
        }
        
        GN_LOG_INFO("Added " + std::to_string(decorations.size()) + 
                   " decorations to group " + std::to_string(groupId));
    }
    
    // STEP 5: Request coin spawning from PickupSystem
    std::vector<Gnosis::Entity> coins = m_pickupSystem->SpawnCoinsForGroup(groupId);
    
    // Add coins to manifest
    auto* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
    for (auto coin : coins) {
        auto* transform = m_ecsSystem->GetComponent<Transform>(coin);
        if (transform && leaderTransform) {
            float offsetX = transform->position.x - leaderTransform->position.x;
            float offsetY = transform->position.y - leaderTransform->position.y;
            
            // Coins are always on layer 5 (PickupsAndEffects)
            int layerOffset = static_cast<int>(RenderLayer::PickupsAndEffects) - 
                             manifest->baseRenderLayer;
            
            manifest->AddMember(coin, offsetX, offsetY, layerOffset, 
                               true, false, false);
        }
    }
    
    GN_LOG_INFO("LevelManager: Spawned group " + std::to_string(groupId) + 
               " with " + std::to_string(manifest->allMembers.size()) + " total members " +
               "(Obstacles: " + std::to_string(manifest->obstacleCount) + 
               ", Decorations: " + std::to_string(manifest->decorationCount) + 
               ", Pickups: " + std::to_string(manifest->pickupCount) + ")");
}

void LevelManager::OnGroupOffScreen(int groupId) {
    // Called by ObstacleSystem when wrap is needed
    auto* manifest = GetGroupManifest(groupId);
    if (!manifest) {
        GN_LOG_WARNING("OnGroupOffScreen: No manifest for group " + std::to_string(groupId));
        return;
    }
    
    // Find rightmost group position
    float rightmostX = GetRightmostGroupPosition();
    
    // Calculate new X based on level's gap width
    float newX = rightmostX + manifest->gapWidth;
    
    GN_LOG_INFO("Wrapping group " + std::to_string(groupId) + 
               " from current to new X=" + std::to_string(newX));
    
    // Execute the wrap
    WrapGroup(groupId, newX);
}

void LevelManager::WrapGroup(int groupId, float newX) {
    auto* manifest = GetGroupManifest(groupId);
    if (!manifest) return;
    
    // STEP 1: Update all member positions (LevelManager coordinates)
    UpdateGroupMemberPositions(groupId, newX);
    
    // STEP 2: Reset obstacle-specific state (ObstacleSystem manages state)
    if (m_obstacleSystem) {
        m_obstacleSystem->ResetGroupState(groupId);
    }
    
    GN_LOG_INFO("Wrapped group " + std::to_string(groupId) + " to X=" + std::to_string(newX));
}

float LevelManager::GetRightmostGroupPosition() const {
    float rightmost = 0.0f;
    
    for (const auto& [groupId, manifest] : m_groupManifests) {
        auto* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest.leaderEntity);
        if (leaderTransform) {
            float groupRight = leaderTransform->position.x + manifest.groupWidth;
            rightmost = std::max(rightmost, groupRight);
        }
    }
    
    return rightmost;
}
```

**Validation**:
- LevelManager orchestrates entire spawn process
- ObstacleSystem and PickupSystem are pure services
- All entities tracked in manifest

---

### **Task 3.3: Update ObstacleSystem::Update() to Detect Rightmost Member** ⭐ **CRITICAL**

**File**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`
**Function**: `Update()`

**IMPORTANT DESIGN CHANGE**: Wrap when **rightmost group member** scrolls off-screen, NOT when leader does!

**Why**: Prevents decorations/coins on the right side of toilets from disappearing while visible.

**Changes**:

```cpp
// BEFORE (lines ~163-177):
// Check leader position only
Transform* transform = m_ecsSystem->GetComponent<Transform>(leaderEntity);
if (transform->position.x < wrapThreshold) {
    needsWrapping = true;
    WrapGroupAroundScreen(groupId, groupPattern);
}

// AFTER:
// Check RIGHTMOST MEMBER position (leader + rightmost offset)
Transform* leaderTransform = m_ecsSystem->GetComponent<Transform>(leaderEntity);
if (!leaderTransform) continue;

// Get manifest to check rightmost member
auto* manifest = m_levelManager->GetGroupManifest(groupId);
if (!manifest) continue;

// Calculate rightmost edge position
float rightmostX = leaderTransform->position.x + manifest->rightmostMemberOffsetX;

// Wrap when rightmost member goes off-screen (not leader!)
if (rightmostX < wrapThreshold) {
    // Notify orchestrator instead of handling wrap ourselves
    if (m_levelManager) {
        m_levelManager->OnGroupOffScreen(groupId);
    } else {
        GN_LOG_WARNING("ObstacleSystem: Cannot wrap group - LevelManager not set!");
    }
}
```

**Delete Entire WrapGroupAroundScreen() Function**:
- This function is no longer needed (~400 lines of complex logic)
- LevelManager::WrapGroup() handles wrapping now
- ObstacleSystem just detects and notifies

**Validation**:
- ObstacleSystem checks rightmost member position
- Notifies LevelManager when rightmost member off-screen
- No visible disappearing of decorations/coins

---

### **Task 3.4: Refactor ObstacleSystem::InitializeForLevel() to Use Orchestrator** ⭐ **CRITICAL**

**File**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`
**Function**: `InitializeForLevel()`
**Current Lines**: ~50-150

**Goal**: Move orchestration to LevelManager. ObstacleSystem::InitializeForLevel() should become minimal.

**BEFORE** (current orchestration in ObstacleSystem):
```cpp
void ObstacleSystem::InitializeForLevel(int levelId, ...) {
    // ... setup code ...
    
    float nextWorldX = 2000.0f;
    for (int i = 0; i < 10; ++i) {  // Spawn 10 initial groups
        int groupId = m_nextGroupId++;
        
        if (m_currentLevelId == 5) {
            SpawnCastlePattern_GoldToiletPair(nextWorldX, groupId);
        } else if (m_currentLevelId == 4) {
            SpawnSnowPattern_ToiletPair(nextWorldX, groupId);
        }
        // ... etc ...
        
        nextWorldX += 2000.0f; // or 1500.0f for snow
    }
}
```

**AFTER** (orchestration in LevelManager):
```cpp
void ObstacleSystem::InitializeForLevel(int levelId, ...) {
    // Only handle ObstacleSystem-specific setup
    m_currentLevelId = levelId;
    m_levelConfig = levelConfig;
    m_baseScale = baseScale;
    m_worldSpeed = levelConfig ? levelConfig->worldSpeed : 200.0f;
    
    // Clear old data
    m_activeObstacles.clear();
    m_wrappedGroups.clear();
    m_nextGroupId = 0;
    
    // DON'T spawn anything - LevelManager will orchestrate spawning
    GN_LOG_INFO("ObstacleSystem initialized for level " + std::to_string(levelId));
}
```

**Add to LevelManager::InitializeLevel()**:
```cpp
void LevelManager::InitializeLevel(int levelId) {
    m_currentLevelId = levelId;
    
    // Clear old manifests
    m_groupManifests.clear();
    
    // Initialize systems (but don't spawn yet)
    if (m_obstacleSystem) {
        m_obstacleSystem->InitializeForLevel(levelId, m_levelConfig, m_baseScale);
    }
    
    if (m_pickupSystem) {
        m_pickupSystem->InitializeForLevel(levelId);
    }
    
    // Determine initial spawn parameters from level config
    int initialGroupCount = 10;
    float startX = 2000.0f;
    float spacing = 2000.0f;
    
    if (levelId == 4) {
        spacing = 1500.0f; // Snow level
    } else if (levelId == 5) {
        spacing = 2000.0f; // Castle level
    }
    // ... other levels ...
    
    // ORCHESTRATE: Spawn initial groups
    float nextX = startX;
    for (int i = 0; i < initialGroupCount; ++i) {
        int groupId = GetNextGroupId();
        SpawnGroup(groupId, nextX);
        nextX += spacing;
    }
    
    GN_LOG_INFO("LevelManager: Initialized level " + std::to_string(levelId) + 
               " with " + std::to_string(initialGroupCount) + " groups");
}
```

**Validation**:
- ObstacleSystem::InitializeForLevel() only sets up internal state
- LevelManager::InitializeLevel() orchestrates all spawning
- Clear separation of concerns

---

### **Task 3.5: Update PickupSystem to Return Coin Entities**

**File**: `src/FloppyTurd/Systems/PickupSystem.h`

**Update Signature**:
```cpp
// BEFORE:
void spawnCoinsForGroup(int groupId);

// AFTER:
std::vector<Gnosis::Entity> SpawnCoinsForGroup(int groupId); // Public, returns entities
```

**File**: `src/FloppyTurd/Systems/PickupSystem.cpp`

**Refactor**:
```cpp
std::vector<Gnosis::Entity> PickupSystem::SpawnCoinsForGroup(int groupId) {
    std::vector<Gnosis::Entity> coins;
    
    if (!m_ecsSystem || !m_levelManager || !m_levelConfig) return coins;
    if (!m_levelManager->GetObstacleSystem()->IsGroupReadyForCoins(groupId)) return coins;

    auto pattern = m_levelManager->GetObstacleSystem()->DetectGroupPattern(groupId);
    auto positions = m_levelManager->GetObstacleSystem()->CalculateCoinPositionsForGroup(groupId, pattern);

    for (const auto& pos : positions) {
        Gnosis::Entity e = m_ecsSystem->CreateEntity();
        if (e == 0) continue;

        // ... create coin with all components ...
        
        // Use enum type instead of string
        const PickupType type = ChoosePickupType(); // Helper returns enum
        
        Pickup pickup;
        pickup.type = type; // NEW: Enum field (no string field!)
        pickup.value = GetValueForPickupType(type);
        // ... rest of setup ...
        
        Sprite sprite;
        if (IsPickupCoinType(type)) {
            sprite = Sprite(ToString(type), 16.0f, 16.0f, 16, 16, 10, 0.1f);
            sprite.isAnimated = true;
        } else {
            sprite = Sprite(ToString(type), 32.0f, 32.0f);
        }
        sprite.layer = static_cast<int>(RenderLayer::PickupsAndEffects); // Always layer 5
        
        // ... add all components ...
        
        coins.push_back(e);
    }
    
    // Store for tracking (PickupSystem still tracks for collection)
    m_groupCoins[groupId] = coins;
    
    // Return to LevelManager for manifest tracking
    return coins;
}

// Helper functions
PickupType PickupSystem::ChoosePickupType() {
    if (!m_levelConfig || m_levelConfig->pickupRatios.empty()) {
        return PickupType::GoldCoin;
    }
    // ... weighted random selection ...
    // Parse string from config, return enum
    if (selected == "GoldCoin") return PickupType::GoldCoin;
    else if (selected == "BlueCoin") return PickupType::BlueCoin;
    // ... etc
    return PickupType::GoldCoin;
}

int PickupSystem::GetValueForPickupType(PickupType type) {
    switch(type) {
        case PickupType::GoldCoin: return 1;
        case PickupType::BlueCoin: return 2;
        case PickupType::RedCoin: return 5;
        default: return 1;
    }
}
```

**Also Fix repositionCoinsForGroup()**:
```cpp
// Line ~413:
s->layer = static_cast<int>(RenderLayer::PickupsAndEffects); // FIX: Always layer 5, not 3!

// Update type field when re-rolling:
p->type = newType; // Enum
// NO p->pickupType string field!
```

**Validation**:
- PickupSystem returns coin entities
- Uses enums, no strings
- Layer 5 always used
- LevelManager can track coins in manifest

---

### **Task 3.6: Update Castle Decoration Spawn Functions**

**File**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`
**Functions**: `SpawnCastleCurtain`, `SpawnCastleFloorTorch`, `SpawnCastleChandelier`, `SpawnCastleTorchPillar`, `SpawnCastleDecorativePainting`, `SpawnCastleSpikeBall`

**Changes Required for EACH function**:

1. **Add Decoration component and use LayerManager**:
```cpp
// After creating entity and adding Transform/Sprite:
Decoration decoration(DecorationType::Curtain, groupId, offsetX, 0.0f, 
                     LayerManager::GetLayerForDecoration(DecorationType::Curtain));
m_ecsSystem->AddComponent<Decoration>(entity, decoration);
```

2. **Use LayerManager for sprite layer**:
```cpp
sprite.layer = LayerManager::GetLayerForDecoration(DecorationType::Curtain);
```

3. **Return entity** (these functions are called by SpawnDecorationsForGroup):
```cpp
return entity; // So LevelManager can track it
```

**Apply to all decoration functions**. For SpikeBall (it's an obstacle):
```cpp
obstacle.type = ObstacleType::SpikeBall;
sprite.layer = LayerManager::GetLayerForObstacle(ObstacleType::SpikeBall);
```

**Validation**:
- All decorations use enums and LayerManager
- Decorations returned to caller

---

### **Task 3.7: Replace All String Comparisons with Enum Checks**

**Files**: `ObstacleSystem.cpp`, `PickupSystem.cpp`

**Find and Replace Pattern**:
```cpp
// BEFORE:
if (obstacle->obstacleType == "TopToilet")
if (type == "GoldCoin")  
if (s->textureId == "Outhouse")

// AFTER:
if (obstacle->type == ObstacleType::TopToilet)
if (type == PickupType::GoldCoin)
// textureId comparisons stay (those are texture names, not types)
```

**Locations to Update**:
- `ObstacleSystem::ResetGroupState()` - use enum comparisons
- `PickupSystem::Update()` - use enum comparisons for pickup collection
- `PickupSystem::repositionCoinsForGroup()` - use enum comparisons
- Any other `.obstacleType ==` or `.pickupType ==` comparisons

**Validation**:
- Global search for `obstacleType ==` returns zero results (except ToString())
- Global search for `pickupType ==` returns zero results
- All logic uses type-safe enums

---

### **Task 3.8: Add GetNextGroupId() to LevelManager**

**File**: `src/FloppyTurd/Systems/LevelManager.h`

```cpp
private:
    int m_nextGroupId = 0;
    
public:
    int GetNextGroupId() { return m_nextGroupId++; }
```

This replaces `ObstacleSystem::m_nextGroupId` since LevelManager now controls group creation.

**Validation**:
- LevelManager tracks group IDs
- ObstacleSystem doesn't need m_nextGroupId anymore

---

### **Task 3.9: Add Sewer Pattern Variation on Wrap** 

**File**: `src/FloppyTurd/Systems/LevelManager.cpp`
**Function**: `WrapGroup()`

**Goal**: When sewer level groups wrap, randomize the pipe pattern for variety.

**Add to WrapGroup() for Sewer level**:
```cpp
void LevelManager::WrapGroup(int groupId, float newX) {
    auto* manifest = GetGroupManifest(groupId);
    if (!manifest) return;
    
    // STEP 1: Update all member positions
    UpdateGroupMemberPositions(groupId, newX);
    
    // STEP 2: Reset obstacle-specific state
    if (m_obstacleSystem) {
        m_obstacleSystem->ResetGroupState(groupId);
        
        // SEWER LEVEL SPECIAL: Randomize pipe pattern on wrap for variety
        if (m_currentLevelId == 2) {
            // Choose random pattern: TopOnly, BottomOnly, or TopAndBottom
            GroupPattern newPattern = static_cast<GroupPattern>(rand() % 3 + 1);
            manifest->pattern = newPattern;
            
            // Respawn pipes with new pattern
            m_obstacleSystem->RespawnPipesForGroup(groupId, newX, newPattern);
            
            GN_LOG_INFO("Sewer wrap: Changed group " + std::to_string(groupId) + 
                       " pattern to " + std::to_string(static_cast<int>(newPattern)));
        }
    }
    
    GN_LOG_INFO("Wrapped group " + std::to_string(groupId) + " to X=" + std::to_string(newX));
}
```

**Add to ObstacleSystem**:
```cpp
void ObstacleSystem::RespawnPipesForGroup(int groupId, float x, GroupPattern newPattern) {
    // Delete old pipe entities in this group
    // Spawn new pipes with new pattern
    // Return new entity vector to LevelManager to update manifest
}
```

**Validation**:
- Sewer level pipes change patterns when wrapping
- Adds gameplay variety
- Other levels unaffected

---

### **Task 3.10: Add Castle Painting Randomization**

**File**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`
**Function**: `SpawnDecorationsForGroup()`

**Goal**: Randomize centerpiece painting each time castle group spawns/wraps.

**Current Code** (line ~2756):
```cpp
// Spawn centerpiece in center of the gap - randomly choose between:
// 1. Torch pillar, 2. Decorative painting, 3. Spike ball obstacle
static int centerCounter = 0;
int centerpieceType = centerCounter % 3; // 3 different centerpiece types
```

**Updated Code**:
```cpp
// Spawn centerpiece in center of the gap - randomly choose between:
// 1. Torch pillar, 2. Random painting, 3. Spike ball obstacle
static int centerCounter = 0;
int centerpieceType = centerCounter % 3; // 3 different centerpiece types

Gnosis::Entity centerpiece = 0;
switch (centerpieceType) {
    case 0:
        centerpiece = SpawnCastleTorchPillar(gapCenterX, groupId, centerOffsetX);
        break;
    case 1: {
        // RANDOMIZE PAINTING - choose one of 4 paintings
        DecorationType paintingType;
        int paintingChoice = rand() % 4;
        switch(paintingChoice) {
            case 0: paintingType = DecorationType::PaintingRabbitKnight; break;
            case 1: paintingType = DecorationType::PaintingRatBeach; break;
            case 2: paintingType = DecorationType::PaintingRiverWalk; break;
            case 3: paintingType = DecorationType::PaintingCabin; break;
            default: paintingType = DecorationType::PaintingRabbitKnight;
        }
        centerpiece = SpawnCastleDecorativePainting(gapCenterX, groupId, centerOffsetX, paintingType);
        break;
    }
    case 2:
        centerpiece = SpawnCastleSpikeBall(gapCenterX, groupId, centerOffsetX);
        break;
}
```

**Update SpawnCastleDecorativePainting() signature**:
```cpp
// BEFORE:
Gnosis::Entity SpawnCastleDecorativePainting(float x, int groupId, float offsetX);

// AFTER:
Gnosis::Entity SpawnCastleDecorativePainting(float x, int groupId, float offsetX, 
                                             DecorationType paintingType = DecorationType::PaintingRabbitKnight);
```

**Update function implementation**:
```cpp
Gnosis::Entity ObstacleSystem::SpawnCastleDecorativePainting(float x, int groupId, float offsetX, 
                                                             DecorationType paintingType) {
    // ... setup code ...
    
    // Use enum to determine texture
    const char* textureName = ToString(paintingType);
    Sprite sprite(textureName, 96.0f, 96.0f);
    sprite.layer = LayerManager::GetLayerForDecoration(paintingType);
    
    // ... rest of code ...
    
    // Add Decoration component with actual painting type
    Decoration decoration(paintingType, groupId, offsetX, 0.0f, sprite.layer);
    m_ecsSystem->AddComponent<Decoration>(painting, decoration);
    
    return painting;
}
```

**Validation**:
- Castle paintings randomize on each spawn/wrap
- All 4 painting variants appear
- Adds visual variety

---

---

## ⭐ **PHASE 5: FUTURE ENHANCEMENTS (OPTIONAL - Post-Refactor)**
**Goal**: Castle level special heart mechanic (every 50 pipes).
**Estimated Time**: 1-2 hours
**Risk Level**: 🟡 Medium (requires extensive testing)
**Status**: NOT PRIORITY - Defer until after refactor validation

---

### **Task 5.1: Castle Rainbow Heart Wave (Every 50 Pipes)**

**Concept**: At pipes 50, 100, 150, etc., spawn a special rainbow heart that oscillates across the screen like toilet paper enemies.

**Implementation**:
```cpp
// In LevelManager or CastleLevel-specific system
void CheckSpecialHeartSpawn(int pipeCount) {
    if (m_currentLevelId != 5) return;
    if (pipeCount % 50 != 0) return;
    
    // Spawn rainbow heart with wave behavior
    Gnosis::Entity heart = SpawnSpecialHeart(PickupType::HeartRainbow);
    
    // Add oscillation behavior (like toilet paper enemies)
    Oscillation osc;
    osc.amplitude = 600.0f;
    osc.frequency = 2.0f;
    osc.horizontal = false; // Vertical waves
    m_ecsSystem->AddComponent<Oscillation>(heart, osc);
    
    // Special effect on pickup: refill hearts + 8s invisibility
}
```

**Effects on Pickup**:
- Refill player hearts to max
- Grant 8 seconds of invisibility
- Play special sound/visual effect

**Testing Required**:
- Play to pipe 50+ in castle level
- Verify heart spawns correctly
- Test oscillation doesn't clip through walls
- Verify invisibility duration

**Note**: This is a future enhancement. Complete and validate the main refactor first!

---

## ✅ **PHASE 4: FINAL VALIDATION & DOCUMENTATION**
**Goal**: Verify refactor completeness, document changes.
**Estimated Time**: 30 minutes
**Risk Level**: 🟢 Low

---

### **Task 4.1: Verify All Legacy Code Removed**

**Check these locations**:
- [ ] `GameComponents.h`: Obstacle struct has NO `obstacleType` string field
- [ ] `GameComponents.h`: Pickup struct has NO `pickupType` string field
- [ ] `ObstacleSystem.h`: NO `m_obstacleGroups` map exists
- [ ] `ObstacleSystem.cpp`: NO references to `m_obstacleGroups`
- [ ] `ObstacleSystem.cpp`: NO string comparisons like `obstacleType ==`
- [ ] `PickupSystem.cpp`: NO string comparisons like `pickupType ==`

**Validation**:
- Global search for `obstacleType` returns only the `ToString()` helper function
- Global search for `m_obstacleGroups` returns zero results

---

### **Task 4.2: Create Completion Summary**

**File**: `OBSTACLE_GROUP_REFACTOR_SUMMARY.md` (new file)

**Contents**:
- What was changed
- Performance impact (should be neutral or better)
- How to use new system
- Known issues (if any)
- Future improvements

---

## 🧪 **TESTING CHECKLIST**

### **Single Comprehensive Test (After All Phases Complete)**

**Build First**:
- [ ] Code compiles without errors or warnings
- [ ] No missing symbol errors
- [ ] All enum conversions compile

**Castle Level (Level 5) - Primary Focus**:
- [ ] Groups spawn correctly with all decorations
- [ ] Coins render on top of decorations (layer 5)
- [ ] Decorations render at correct layers (curtains=2, torches/chandeliers=3)
- [ ] Groups wrap correctly with ALL members moving together
- [ ] Coins stay centered in gaps between toilet pairs
- [ ] Decorations stay centered/positioned correctly
- [ ] Pipe counter increments past 100+ pipes
- [ ] No desync between any group members
- [ ] No visual glitches or disappearing elements
- [ ] Spike balls included in groups and wrap correctly

**Snow Level (Level 4) - Verification**:
- [ ] Toilet pairs spawn with consistent 1500px gaps
- [ ] Coins positioned correctly in gaps
- [ ] Groups wrap smoothly
- [ ] No regression from previous work

**Other Levels - Regression Check**:
- [ ] Park level (Level 1): Toilets spawn and wrap normally
- [ ] Sewer level (Level 2): Pipes spawn normally
- [ ] Desert level (Level 3): Outhouses spawn normally
- [ ] Boss level (Level 6): RatKing unaffected, works normally

**Performance & Stability**:
- [ ] 60fps maintained on all levels
- [ ] No memory leaks during extended play
- [ ] Log output clean (no errors/warnings about groups)
- [ ] No crashes during 100+ pipe runs

**Debug Validation** (if enabled):
- [ ] `ValidateGroupIntegrity()` shows no desyncs
- [ ] All group members present and accounted for
- [ ] Relative offsets correct for all members

---

## 📊 **SUCCESS METRICS**

- ✅ No visual glitches or desyncs after 100+ pipes in castle level
- ✅ Coins always render on layer 5 (above decorations)
- ✅ All decorations move with their groups during wrapping
- ✅ No string comparisons in hot paths (wrapping, update loops)
- ✅ GroupManifest contains all group members (obstacles, decorations, pickups)
- ✅ Code is easier to understand and maintain
- ✅ Adding new obstacle/decoration types is straightforward
- ✅ No performance regression

---

## 🚨 **ROLLBACK PLAN**

If critical issues arise:

1. **Phase 3 Issues**: Revert to Phase 2 (enums defined but not used)
2. **Phase 2 Issues**: Revert to Phase 1 (only bug fixes applied)
3. **Phase 1 Issues**: Revert coin layer change only

**Git Strategy**:
- Commit after each task completion
- Tag each phase completion
- Can cherry-pick individual fixes if needed

---

## 📝 **NOTES & DECISIONS**

- **No JSON configs**: All hardcoded values preserved in code
- **Backward compatibility**: Legacy string fields kept during migration
- **Performance**: Manifest lookups are O(1) hash map access
- **Memory**: Minimal overhead (~100 bytes per group manifest)
- **Debugging**: Validation methods available for troubleshooting

---

## 🎯 **READY TO START?**

Once approved, we'll proceed through ALL phases without stopping to test. Expected total time: **4-6 hours** in one continuous session.

**Approach**: Complete all phases → Compile once → Test once → Debug if needed

**First task**: Phase 1 Task 1.1 (Fix coin layer bug)
**Last task**: Phase 4 Task 4.2 (Create summary document)

---

## 📋 **KEY ARCHITECTURAL CHANGES SUMMARY**

### **What's Moving**:
| Component | Old Location | New Location | Reason |
|-----------|-------------|--------------|--------|
| `GroupManifest` storage | ObstacleSystem | **LevelManager** | Coordinates obstacles, decorations, AND pickups |
| Type identification | String fields | **Enum fields** | Type safety, performance |
| Layer assignment | Hardcoded per spawn | **LayerManager** | Centralized, consistent |
| Group member tracking | `m_obstacleGroups` | **GroupManifest** | Unified, explicit |

### **What's Being Deleted**:
- ❌ `Obstacle::obstacleType` (string field)
- ❌ `Pickup::pickupType` (string field)  
- ❌ `ObstacleSystem::m_obstacleGroups` (legacy map)
- ❌ All string comparison logic (`if (type == "GoldCoin")`)

### **What's Being Added**:
- ✅ `ObstacleType`, `DecorationType`, `PickupType` enums
- ✅ `Decoration` component for castle decorations
- ✅ `GroupManifest` struct with cross-system member tracking
- ✅ `LayerManager` for centralized render layer control
- ✅ `LevelManager` methods: `CreateGroupManifest()`, `UpdateGroupMemberPositions()`, `ValidateGroupIntegrity()`

### **Communication Flow (After Refactor)**:

#### **INITIALIZATION (LevelManager Orchestrates)**:
```
LevelManager::InitializeLevel(levelId)
  ↓
Reads level config: pattern, gaps, spawn count
  ↓
FOR EACH initial group (e.g., 10 groups):
  ↓
  LevelManager::SpawnGroup(groupId, xPosition)
    ↓
    1. CreateGroupManifest(groupId, pattern, gapWidth, toiletWidth)
       [LevelManager creates and owns the manifest]
    ↓
    2. Request toilet spawning:
       entities = ObstacleSystem->SpawnToiletPairForGroup(groupId, x, pattern)
       [ObstacleSystem creates toilets, returns entity IDs]
    ↓
    3. Add toilets to manifest:
       manifest->AddMember(entities[0], offsetX, offsetY, ...)
       [LevelManager populates its own manifest]
    ↓
    4. IF castle level, request decorations:
       decorations = ObstacleSystem->SpawnDecorationsForGroup(groupId, x, pattern)
       [ObstacleSystem creates decorations, returns entity IDs]
    ↓
    5. Add decorations to manifest:
       FOR EACH decoration: manifest->AddMember(...)
       [LevelManager tracks everything]
    ↓
    6. Request coin spawning:
       coins = PickupSystem->SpawnCoinsForGroup(groupId)
       [PickupSystem creates coins using ObstacleSystem's position data]
    ↓
    7. Add coins to manifest:
       FOR EACH coin: manifest->AddMember(...)
       [LevelManager now has complete group tracking]
```

#### **RUNTIME (LevelManager Coordinates)**:
```
ObstacleSystem::Update() checks group positions
  ↓
Detects group leader off-screen (x < wrapThreshold)
  ↓
Notifies orchestrator: m_levelManager->OnGroupOffScreen(groupId)
  [ObstacleSystem is just a sensor - doesn't handle wrapping]
  ↓
LevelManager::OnGroupOffScreen(groupId)
  ↓
  1. Find rightmost group for positioning
  2. Calculate newX based on level's gap width
  3. WrapGroup(groupId, newX)
     ↓
     UpdateGroupMemberPositions(groupId, newX)
       [Moves ALL members: toilets + decorations + coins]
     ↓
     ObstacleSystem->ResetGroupState(groupId)
       [Resets pipeCleared, randomizes Y, resets oscillation]
```

**Result**: 
- **LevelManager** = Owner & Orchestrator (owns manifests, controls lifecycle)
- **ObstacleSystem** = Service (creates entities, detects events, manages behavior)
- **PickupSystem** = Service (creates coins on demand)
- **Perfect Sync** = Single source of truth coordinates all systems

---

**END OF REFACTORING PLAN**

