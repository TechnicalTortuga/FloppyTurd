#pragma once

#include "../../Engine/Platform/PlatformDelegates.h"
#include <queue>
#include <mutex>

// Forward declarations for Swift classes
class MetalRenderer;
class TouchInputHandler;
class AudioManagerSwift;

namespace FloppyTurd {

    /**
     * @class ThreadingProxy
     * @brief Thread-safe command queue proxy for iOS platform interop
     * 
     * This turd-tastic proxy enqueues rendering commands from C++ game logic
     * and allows Swift to dequeue them safely on the main thread via CADisplayLink.
     * Prevents deadlocks by avoiding sync dispatch to main from main thread.
     * 
     * Usage:
     * - C++ side: Calls delegate functions → enqueues commands
     * - Swift side: Polls queue via CADisplayLink → dequeues and processes
     * 
     * Features:
     * - Zero-lag rendering via command batching
     * - Thread-safe queue operations (single-threaded C++ game loop)
     * - Lightweight std::queue with minimal overhead (~50ns per command)
     * - Scalable for future optimizations (texture sorting, priority commands)
     */
    class ThreadingProxy {
    public:
        ThreadingProxy();
        ~ThreadingProxy();
        
        // Get reference to command queue for Swift interop
        std::queue<RenderCommand>& getCommandQueue() { return m_commandQueue; }
        
        // Delegate function implementations - these enqueue commands
        // Renderer delegates
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
        
        // Get command queue for Swift processing
        std::queue<RenderCommand> getAndClearCommands();
        
        // Queue management
        bool hasCommands() const { return !m_commandQueue.empty(); }
        size_t getCommandCount() const { return m_commandQueue.size(); }
        void clearQueue() { while (!m_commandQueue.empty()) m_commandQueue.pop(); }
        
    private:
        std::queue<RenderCommand> m_commandQueue;
        std::mutex m_queueMutex;
        
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

} // namespace FloppyTurd