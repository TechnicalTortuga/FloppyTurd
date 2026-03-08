#pragma once

#include "Event.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace Gnosis {

    // Type alias for event handlers - using function pointer for compatibility
    typedef void (*EventHandler)(const Event&);
    
    /**
     * Manages event dispatching and subscription in the game.
     * Allows systems to subscribe to specific event types and dispatch events.
     */
    class EventManager {
    private:
        // Map of event types to their handlers
        std::unordered_map<EventType, std::vector<EventHandler> > eventHandlers;
        
        // Queue of events to be processed
        std::vector<std::unique_ptr<Event> > eventQueue;
        
        // Maximum number of events to process per frame
        static const size_t MAX_EVENTS_PER_FRAME = 100;
        
    public:
        EventManager() {}
        
        ~EventManager() {
            Clear();
        }
        
        /**
         * Subscribe to an event type with a handler function
         */
        void Subscribe(EventType eventType, const EventHandler& handler) {
            eventHandlers[eventType].push_back(handler);
        }
        
        /**
         * Dispatch an event immediately to all subscribers
         */
        void DispatchEvent(const Event& event) {
            std::unordered_map<EventType, std::vector<EventHandler> >::iterator it = eventHandlers.find(event.type);
            if (it != eventHandlers.end()) {
                for (std::vector<EventHandler>::const_iterator handlerIt = it->second.begin(); handlerIt != it->second.end(); ++handlerIt) {
                    (*handlerIt)(event);
                }
            }
        }
        
        /**
         * Queue an event to be processed later
         */
        void QueueEvent(std::unique_ptr<Event> event) {
            if (eventQueue.size() < MAX_EVENTS_PER_FRAME * 2) { // Allow some buffer
                eventQueue.push_back(std::move(event));
            }
        }
        
        /**
         * Process all queued events
         */
        void ProcessEvents() {
            size_t eventsProcessed = 0;
            
            while (!eventQueue.empty() && eventsProcessed < MAX_EVENTS_PER_FRAME) {
                std::unique_ptr<Event> event = std::move(eventQueue.front());
                eventQueue.erase(eventQueue.begin());
                
                DispatchEvent(*event);
                eventsProcessed++;
            }
        }
        
        /**
         * Clear all queued events
         */
        void Clear() {
            eventQueue.clear();
        }
        
        /**
         * Remove all subscribers for a specific event type
         */
        void ClearSubscribers(EventType eventType) {
            auto it = eventHandlers.find(eventType);
            if (it != eventHandlers.end()) {
                it->second.clear();
            }
        }
        
        /**
         * Remove all subscribers for all event types
         */
        void ClearAllSubscribers() {
            eventHandlers.clear();
        }
        
        /**
         * Get the number of queued events
         */
        size_t GetQueuedEventCount() const {
            return eventQueue.size();
        }
        
        /**
         * Get the number of subscribers for an event type
         */
        size_t GetSubscriberCount(EventType eventType) const {
            auto it = eventHandlers.find(eventType);
            return (it != eventHandlers.end()) ? it->second.size() : 0;
        }
        
        /**
         * Helper methods for common events
         */
        
        void FireCollisionEvent(Entity entityA, Entity entityB, const GNVector2& point = GNVector2(), const GNVector2& normal = GNVector2()) {
            auto event = std::unique_ptr<Event>(new CollisionEvent(entityA, entityB, point, normal));
            QueueEvent(std::move(event));
        }
        
        void FireScoreEvent(int points, int totalScore, Entity entity = INVALID_ENTITY) {
            auto event = std::unique_ptr<Event>(new ScoreEvent(points, totalScore, entity));
            QueueEvent(std::move(event));
        }
        
        void FirePlayerDeathEvent(Entity player, Entity killer = INVALID_ENTITY, const std::string& cause = "") {
            auto event = std::unique_ptr<Event>(new PlayerDeathEvent(player, killer, cause));
            QueueEvent(std::move(event));
        }
        
        void FireLevelCompleteEvent(int levelId, float completionTime, int finalScore) {
            auto event = std::unique_ptr<Event>(new LevelCompleteEvent(levelId, completionTime, finalScore));
            QueueEvent(std::move(event));
        }
        
        void FireJumpEvent(Entity entity, const GNVector2& position, float force) {
            auto event = std::unique_ptr<Event>(new JumpEvent(entity, position, force));
            QueueEvent(std::move(event));
        }
        
        void FireEnemyKilledEvent(Entity enemy, Entity killer, int scoreReward) {
            auto event = std::unique_ptr<Event>(new EnemyKilledEvent(enemy, killer, scoreReward));
            QueueEvent(std::move(event));
        }
        
        void FireTelemetryEvent(GameEvent gameEvent, const std::map<std::string, std::string>& metadata = std::map<std::string, std::string>()) {
            auto event = std::unique_ptr<Event>(new TelemetryEvent(gameEvent, metadata));
            QueueEvent(std::move(event));
        }
    };
    
} // namespace Gnosis