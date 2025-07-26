#pragma once

#include "../../Engine/Platform/PlatformDelegates.h"
#include <vector>  // For std::vector (bridges to Swift Array)
#include <mutex>
#include <queue>  // Internal queue, but we return vector for interop

// Forward declarations for Swift classes
class MetalRenderer;
class TouchInputHandler;
class AudioManagerSwift;

namespace FloppyTurd {

    /**
     * @class ThreadingProxy
     * @brief Thread-safe command queue proxy for iOS platform interop
     * 
     * Enqueues rendering commands from C++ game logic and allows Swift to dequeue them safely on the main thread.
     * Follows Apple's C++/Swift interop best practices: Simple POD structs for commands, std::vector for bridging to Array,
     * no unions (flattened data), and mutex protection for thread safety (though C++ game loop is single-threaded).
     * 
     * Usage:
     * - C++: Enqueue via delegates.
     * - Swift: Get vector of commands via getAndClearCommands() (UnsafePointer to call), process on main.
     * 
     * Features:
     * - Batch dequeuing via vector for efficient interop (avoids per-command calls).
     * - Flattened command data to avoid union limitations in interop.
     * - Scalable for optimizations like command sorting.
     */
    class ThreadingProxy {
    public:
        ThreadingProxy();
        ~ThreadingProxy();
        
        // Delegate function implementations - these enqueue commands
        static void enqueueBeginFrame();
        static void enqueueEndFrame();
        static void enqueuePresent();
        static void enqueueClearScreen(float r, float g, float b, float a);
        static void enqueueDrawSprite(void* sprite, float x, float y, float rotation);
        static void enqueueDrawSpriteScaled(void* sprite, float x, float y, float scaleX, float scaleY, float rotation);
        static void enqueueDrawText(const char* text, float x, float y, float fontSize, float r, float g, float b, float a);
        static void enqueueDrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a);
        static void enqueueDrawCircle(float x, float y, float radius, float r, float g, float b, float a);
        static void enqueueGetScreenSize(float* width, float* height);
        
        // Setup function to configure delegates to use this proxy
        void setupDelegates(PlatformDelegates& delegates);
        
        // Set Swift components for command processing
        void setSwiftComponents(MetalRenderer* renderer, TouchInputHandler* input, AudioManagerSwift* audio);
        
        // Get and clear commands as vector (bridges to Swift Array<RenderCommand>)
        std::vector<RenderCommand> getAndClearCommands();
        
        // Queue management - Thread-safe
        bool hasCommands() const;
        size_t getCommandCount() const;
        void clearQueue();
        
    private:
        std::queue<RenderCommand> m_commandQueue;
        mutable std::mutex m_queueMutex;  // mutable for const methods
        
        // Swift component references
        MetalRenderer* m_metalRenderer;
        TouchInputHandler* m_touchInputHandler;
        AudioManagerSwift* m_audioManager;
        
        // Static instance for delegate callbacks
        static ThreadingProxy* s_instance;
        
        // Helper to enqueue a command
        void enqueueCommand(const RenderCommand& command);
    };
    
    // Global instance accessor for C++ interop
    extern ThreadingProxy* g_threadingProxy;
    
    // Swift-accessible functions for threading system management
    void initializeThreadingSystem();
    void shutdownThreadingSystem();
    ThreadingProxy* getThreadingProxy();
    
    // Helper function for Swift to get commands without dealing with C++ method calls
    std::vector<RenderCommand> getAndClearCommandsFromProxy();

} // namespace FloppyTurd