#pragma once

#include "../Core/GnosisTypes.h"
#include <string>
#include <map>

namespace Gnosis {

    /**
     * Base class for all events in the system
     */
    struct Event {
        EventType type;
        
        Event(EventType eventType) : type(eventType) {}
        virtual ~Event() = default;
    };
    
    /**
     * Collision event - fired when two entities collide
     */
    struct CollisionEvent : public Event {
        Entity entityA;
        Entity entityB;
        GNVector2 collisionPoint;
        GNVector2 collisionNormal;
        
        CollisionEvent(Entity a, Entity b, const GNVector2& point = GNVector2(), const GNVector2& normal = GNVector2()) throw()
            : Event(EventType::COLLISION), entityA(a), entityB(b), collisionPoint(point), collisionNormal(normal) {}
    };
    
    /**
     * Score event - fired when the player's score changes
     */
    struct ScoreEvent : public Event {
        int points;
        int totalScore;
        Entity scoringEntity;
        
        ScoreEvent(int pts, int total, Entity entity = INVALID_ENTITY)
            : Event(EventType::SCORE_INCREASED), points(pts), totalScore(total), scoringEntity(entity) {}
    };
    
    /**
     * Player death event
     */
    struct PlayerDeathEvent : public Event {
        Entity playerEntity;
        Entity killerEntity;
        std::string deathCause;
        
        PlayerDeathEvent(Entity player, Entity killer = INVALID_ENTITY, const std::string& cause = "")
            : Event(EventType::PLAYER_DIED), playerEntity(player), killerEntity(killer), deathCause(cause) {}
    };
    
    /**
     * Level complete event
     */
    struct LevelCompleteEvent : public Event {
        int levelId;
        float completionTime;
        int finalScore;
        
        LevelCompleteEvent(int id, float time, int score)
            : Event(EventType::LEVEL_COMPLETE), levelId(id), completionTime(time), finalScore(score) {}
    };
    
    /**
     * Jump performed event
     */
    struct JumpEvent : public Event {
        Entity jumpingEntity;
        GNVector2 jumpPosition;
        float jumpForce;
        
        JumpEvent(Entity entity, const GNVector2& position, float force)
            : Event(EventType::JUMP_PERFORMED), jumpingEntity(entity), jumpPosition(position), jumpForce(force) {}
    };
    
    /**
     * Enemy killed event
     */
    struct EnemyKilledEvent : public Event {
        Entity enemyEntity;
        Entity killerEntity;
        int scoreReward;
        
        EnemyKilledEvent(Entity enemy, Entity killer, int score)
            : Event(EventType::ENEMY_KILLED), enemyEntity(enemy), killerEntity(killer), scoreReward(score) {}
    };
    
    /**
     * Telemetry event - for analytics and tracking
     */
    struct TelemetryEvent : public Event {
        GameEvent gameEvent;
        std::map<std::string, std::string> metadata;
        
        TelemetryEvent(GameEvent event, const std::map<std::string, std::string>& data = std::map<std::string, std::string>()) throw()
            : Event(EventType::SCORE_INCREASED), gameEvent(event), metadata(data) {} // Note: Using SCORE_INCREASED as placeholder
    };
    
} // namespace Gnosis