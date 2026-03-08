#pragma once

#include <vector>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace GameCore {
    
    /**
     * @struct TouchData
     * @brief Represents a single touch point with position, pressure, and timing data
     */
    struct TouchData {
        int touchId;
        float x, y;
        float pressure;
        uint64_t timestamp;
        bool isActive;
        
        TouchData() : touchId(-1), x(0), y(0), pressure(1.0f), timestamp(0), isActive(false) {}
        TouchData(int id, float posX, float posY, float press, uint64_t time, bool active)
            : touchId(id), x(posX), y(posY), pressure(press), timestamp(time), isActive(active) {}
    };
    
    /**
     * @struct GestureData
     * @brief Represents gesture recognition data with type, position, and confidence
     */
    struct GestureData {
        enum Type { 
            NONE = 0,
            TAP = 1, 
            SWIPE_UP = 2, 
            SWIPE_DOWN = 3, 
            SWIPE_LEFT = 4, 
            SWIPE_RIGHT = 5, 
            PINCH = 6, 
            ROTATION = 7 
        };
        
        Type type;
        float x, y;
        float confidence;
        uint64_t timestamp;
        
        GestureData() : type(NONE), x(0), y(0), confidence(0), timestamp(0) {}
        GestureData(Type gestureType, float posX, float posY, float conf, uint64_t time)
            : type(gestureType), x(posX), y(posY), confidence(conf), timestamp(time) {}
    };
    
    /**
     * @struct InputFrame
     * @brief Represents all input data for a single frame
     */
    struct InputFrame {
        uint64_t frameNumber;
        uint64_t timestamp;
        std::vector<TouchData> touches;
        std::vector<GestureData> gestures;
        
        InputFrame() : frameNumber(0), timestamp(0) {}
        InputFrame(uint64_t frame, uint64_t time) : frameNumber(frame), timestamp(time) {}
    };
    
    /**
     * @class InputStateBuffer
     * @brief Thread-safe buffer for storing input history and state for combo detection and analysis
     * 
     * This class maintains a rolling buffer of input frames to support:
     * - Combo detection with timing windows
     * - Input sequence analysis
     * - Input state queries for polling-based game logic
     * - Input analytics and debugging
     * 
     * Thread Safety: All public methods are thread-safe via mutex protection
     */
    class InputStateBuffer {
    private:
        std::deque<InputFrame> m_inputHistory;
        InputFrame m_currentFrame;
        mutable std::mutex m_bufferMutex;
        uint64_t m_currentFrameNumber;
        
        static constexpr size_t MAX_HISTORY_FRAMES = 120; // 2 seconds at 60fps
        static constexpr float COMBO_TIME_WINDOW = 2.0f;  // 2 seconds for combo detection
        
    public:
        /**
         * @brief Constructor
         */
        InputStateBuffer();
        
        /**
         * @brief Destructor
         */
        ~InputStateBuffer() = default;
        
        // Thread-safe input recording methods
        
        /**
         * @brief Record a touch event in the current frame
         * @param touchId Unique identifier for the touch
         * @param x X coordinate (normalized 0-1 or screen coordinates)
         * @param y Y coordinate (normalized 0-1 or screen coordinates)
         * @param pressure Touch pressure (0.0-1.0)
         */
        void recordTouch(int touchId, float x, float y, float pressure);
        
        /**
         * @brief Record a gesture event in the current frame
         * @param type Type of gesture detected
         * @param x X coordinate of gesture center
         * @param y Y coordinate of gesture center
         * @param confidence Confidence level of gesture recognition (0.0-1.0)
         */
        void recordGesture(GestureData::Type type, float x, float y, float confidence);
        
        /**
         * @brief Advance to the next frame, archiving current frame data
         * Should be called once per game frame
         */
        void advanceFrame();
        
        // Input state query methods (for game logic polling)
        
        /**
         * @brief Check if a specific touch is currently active
         * @param touchId Touch identifier to check
         * @return true if touch is active in current frame
         */
        bool isTouchActive(int touchId) const;
        
        /**
         * @brief Get touch data for a specific touch ID
         * @param touchId Touch identifier
         * @return TouchData for the specified touch, or default TouchData if not found
         */
        TouchData getTouchData(int touchId) const;
        
        /**
         * @brief Get all gestures that occurred within a time window
         * @param seconds Time window in seconds (looking back from current frame)
         * @return Vector of GestureData within the time window
         */
        std::vector<GestureData> getGesturesInTimeWindow(float seconds) const;
        
        /**
         * @brief Get input sequence for combo detection
         * @param timeWindow Time window in seconds to look back
         * @return Vector of gestures in chronological order within time window
         */
        std::vector<GestureData> getInputSequence(float timeWindow) const;
        
        /**
         * @brief Get all active touches in the current frame
         * @return Vector of all active TouchData
         */
        std::vector<TouchData> getActiveTouches() const;
        
        /**
         * @brief Get the current frame number
         * @return Current frame number
         */
        uint64_t getCurrentFrameNumber() const;
        
        /**
         * @brief Get the number of frames stored in history
         * @return Number of historical frames available
         */
        size_t getHistorySize() const;
        
        /**
         * @brief Clear all input history and reset state
         */
        void clear();
        
        // Debug and analytics methods
        
        /**
         * @brief Get input statistics for the last N frames
         * @param frameCount Number of frames to analyze
         * @return String containing input statistics
         */
        std::string getInputStats(size_t frameCount = 60) const;
        
    private:
        /**
         * @brief Get current timestamp in milliseconds
         * @return Current timestamp
         */
        uint64_t getCurrentTimestamp() const;
        
        /**
         * @brief Cleanup old frames beyond MAX_HISTORY_FRAMES
         */
        void cleanupOldFrames();
    };
    
} // namespace GameCore
