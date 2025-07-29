#include "InputStateBuffer.h"
#include <chrono>
#include <algorithm>
#include <sstream>

namespace GameCore {
    
    InputStateBuffer::InputStateBuffer() 
        : m_currentFrameNumber(0) {
        m_currentFrame = InputFrame(0, getCurrentTimestamp());
    }
    
    void InputStateBuffer::recordTouch(int touchId, float x, float y, float pressure) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        TouchData touchData(touchId, x, y, pressure, getCurrentTimestamp(), true);
        
        // Update existing touch or add new one
        auto it = std::find_if(m_currentFrame.touches.begin(), m_currentFrame.touches.end(),
            [touchId](const TouchData& touch) { return touch.touchId == touchId; });
        
        if (it != m_currentFrame.touches.end()) {
            *it = touchData;
        } else {
            m_currentFrame.touches.push_back(touchData);
        }
    }
    
    void InputStateBuffer::recordGesture(GestureData::Type type, float x, float y, float confidence) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        GestureData gestureData(type, x, y, confidence, getCurrentTimestamp());
        m_currentFrame.gestures.push_back(gestureData);
    }
    
    void InputStateBuffer::advanceFrame() {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        // Archive current frame
        m_currentFrame.frameNumber = m_currentFrameNumber;
        m_currentFrame.timestamp = getCurrentTimestamp();
        m_inputHistory.push_back(m_currentFrame);
        
        // Cleanup old frames
        cleanupOldFrames();
        
        // Start new frame
        m_currentFrameNumber++;
        m_currentFrame = InputFrame(m_currentFrameNumber, getCurrentTimestamp());
    }
    
    bool InputStateBuffer::isTouchActive(int touchId) const {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        auto it = std::find_if(m_currentFrame.touches.begin(), m_currentFrame.touches.end(),
            [touchId](const TouchData& touch) { return touch.touchId == touchId && touch.isActive; });
        
        return it != m_currentFrame.touches.end();
    }
    
    TouchData InputStateBuffer::getTouchData(int touchId) const {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        auto it = std::find_if(m_currentFrame.touches.begin(), m_currentFrame.touches.end(),
            [touchId](const TouchData& touch) { return touch.touchId == touchId; });
        
        if (it != m_currentFrame.touches.end()) {
            return *it;
        }
        
        return TouchData(); // Return default TouchData if not found
    }
    
    std::vector<GestureData> InputStateBuffer::getGesturesInTimeWindow(float seconds) const {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        std::vector<GestureData> result;
        uint64_t currentTime = getCurrentTimestamp();
        uint64_t timeWindowMs = static_cast<uint64_t>(seconds * 1000.0f);
        uint64_t cutoffTime = currentTime - timeWindowMs;
        
        // Check current frame first
        for (const auto& gesture : m_currentFrame.gestures) {
            if (gesture.timestamp >= cutoffTime) {
                result.push_back(gesture);
            }
        }
        
        // Check historical frames
        for (auto it = m_inputHistory.rbegin(); it != m_inputHistory.rend(); ++it) {
            if (it->timestamp < cutoffTime) {
                break; // Frames are ordered, so we can stop here
            }
            
            for (const auto& gesture : it->gestures) {
                if (gesture.timestamp >= cutoffTime) {
                    result.push_back(gesture);
                }
            }
        }
        
        // Sort by timestamp (oldest first)
        std::sort(result.begin(), result.end(), 
            [](const GestureData& a, const GestureData& b) {
                return a.timestamp < b.timestamp;
            });
        
        return result;
    }
    
    std::vector<GestureData> InputStateBuffer::getInputSequence(float timeWindow) const {
        return getGesturesInTimeWindow(timeWindow);
    }
    
    std::vector<TouchData> InputStateBuffer::getActiveTouches() const {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        std::vector<TouchData> activeTouches;
        for (const auto& touch : m_currentFrame.touches) {
            if (touch.isActive) {
                activeTouches.push_back(touch);
            }
        }
        
        return activeTouches;
    }
    
    uint64_t InputStateBuffer::getCurrentFrameNumber() const {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        return m_currentFrameNumber;
    }
    
    size_t InputStateBuffer::getHistorySize() const {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        return m_inputHistory.size();
    }
    
    void InputStateBuffer::clear() {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        m_inputHistory.clear();
        m_currentFrame = InputFrame(0, getCurrentTimestamp());
        m_currentFrameNumber = 0;
    }
    
    std::string InputStateBuffer::getInputStats(size_t frameCount) const {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        std::ostringstream stats;
        stats << "InputStateBuffer Stats:\n";
        stats << "  Current Frame: " << m_currentFrameNumber << "\n";
        stats << "  History Size: " << m_inputHistory.size() << " frames\n";
        stats << "  Active Touches: " << m_currentFrame.touches.size() << "\n";
        stats << "  Current Frame Gestures: " << m_currentFrame.gestures.size() << "\n";
        
        // Count gestures in recent frames
        size_t totalGestures = 0;
        size_t framesToCheck = std::min(frameCount, m_inputHistory.size());
        
        for (size_t i = 0; i < framesToCheck; ++i) {
            const auto& frame = m_inputHistory[m_inputHistory.size() - 1 - i];
            totalGestures += frame.gestures.size();
        }
        
        stats << "  Gestures in last " << framesToCheck << " frames: " << totalGestures << "\n";
        
        return stats.str();
    }
    
    uint64_t InputStateBuffer::getCurrentTimestamp() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
    
    void InputStateBuffer::cleanupOldFrames() {
        while (m_inputHistory.size() > MAX_HISTORY_FRAMES) {
            m_inputHistory.pop_front();
        }
    }
    
} // namespace GameCore
