#include "ThreadingProxy.h"
#include <iostream>

namespace FloppyTurd {

    // Static instance for delegate callbacks
    ThreadingProxy* ThreadingProxy::s_instance = nullptr;
    
    // Global instance for C++ interop
    ThreadingProxy* g_threadingProxy = nullptr;

    ThreadingProxy::ThreadingProxy() 
        : m_metalRenderer(nullptr)
        , m_touchInputHandler(nullptr)
        , m_audioManager(nullptr) {
        s_instance = this;
        g_threadingProxy = this;
        std::cout << "[ThreadingProxy] Turd-tastic proxy initialized! Ready to enqueue commands! 🚀" << std::endl;
    }

    ThreadingProxy::~ThreadingProxy() {
        clearQueue();
        if (s_instance == this) {
            s_instance = nullptr;
        }
        if (g_threadingProxy == this) {
            g_threadingProxy = nullptr;
        }
        std::cout << "[ThreadingProxy] Proxy shutdown - all turds flushed from queue!" << std::endl;
    }

    void ThreadingProxy::enqueueCommand(const RenderCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_commandQueue.push(command);
        // Uncomment for debug logging:
        // std::cout << "[ThreadingProxy] Enqueued command type: " << command.type << " (Queue size: " << m_commandQueue.size() << ")" << std::endl;
    }

    // Delegate function implementations - these enqueue commands
    void ThreadingProxy::enqueueBeginFrame() {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_BEGIN_FRAME;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueEndFrame() {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_END_FRAME;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueuePresent() {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_PRESENT;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueClearScreen(float r, float g, float b, float a) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_CLEAR_SCREEN;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawSprite(void* sprite, float x, float y, float rotation) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_SPRITE;
        cmd.data.sprite = sprite;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.rotation = rotation;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawSpriteScaled(void* sprite, float x, float y, float scaleX, float scaleY, float rotation) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_SPRITE_SCALED;
        cmd.data.sprite = sprite;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.scaleX = scaleX;
        cmd.data.scaleY = scaleY;
        cmd.data.rotation = rotation;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawText(const char* text, float x, float y, float fontSize, float r, float g, float b, float a) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_TEXT;
        cmd.data.text = text;  // Note: Caller must ensure string lifetime
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.fontSize = fontSize;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_RECTANGLE;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.width = width;
        cmd.data.height = height;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_CIRCLE;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.radius = radius;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueGetScreenSize(float* width, float* height) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_GET_SCREEN_SIZE;
        cmd.data.screenWidth = width;
        cmd.data.screenHeight = height;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::setSwiftComponents(MetalRenderer* renderer, TouchInputHandler* input, AudioManagerSwift* audio) {
        m_metalRenderer = renderer;
        m_touchInputHandler = input;
        m_audioManager = audio;
    }
    
    std::vector<RenderCommand> ThreadingProxy::getAndClearCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<RenderCommand> commands;
        while (!m_commandQueue.empty()) {
            commands.push_back(m_commandQueue.front());
            m_commandQueue.pop();
        }
        return commands;  // Bridges to Array<RenderCommand> in Swift
    }
    
    bool ThreadingProxy::hasCommands() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return !m_commandQueue.empty();
    }
    
    size_t ThreadingProxy::getCommandCount() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_commandQueue.size();
    }
    
    void ThreadingProxy::clearQueue() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_commandQueue.empty()) {
            m_commandQueue.pop();
        }
    }

    void ThreadingProxy::setupDelegates(PlatformDelegates& delegates) {
        std::cout << "[ThreadingProxy] Setting up delegates to use command queue - turds will fly smoothly!" << std::endl;
        
        // Configure renderer delegates to use our enqueue functions
        delegates.renderer.beginFrame = enqueueBeginFrame;
        delegates.renderer.endFrame = enqueueEndFrame;
        delegates.renderer.present = enqueuePresent;
        delegates.renderer.clearScreen = enqueueClearScreen;
        delegates.renderer.drawSprite = enqueueDrawSprite;
        delegates.renderer.drawSpriteScaled = enqueueDrawSpriteScaled;
        delegates.renderer.drawText = enqueueDrawText;
        delegates.renderer.drawRectangle = enqueueDrawRectangle;
        delegates.renderer.drawCircle = enqueueDrawCircle;
        delegates.renderer.getScreenSize = enqueueGetScreenSize;
        
        std::cout << "[ThreadingProxy] Delegates configured successfully - ready for turd-tossing action!" << std::endl;
    }

} // namespace FloppyTurd

// Native C++ interface for Swift interop (no extern C needed)
namespace FloppyTurd {
    
    // Swift-accessible functions for threading system management
    void initializeThreadingSystem() {
        if (!g_threadingProxy) {
            g_threadingProxy = new ThreadingProxy();
            std::cout << "[ThreadingProxy] Threading system initialized from Swift!" << std::endl;
        }
    }
    
    void shutdownThreadingSystem() {
        if (g_threadingProxy) {
            delete g_threadingProxy;
            g_threadingProxy = nullptr;
            std::cout << "[ThreadingProxy] Threading system shutdown from Swift!" << std::endl;
        }
    }
    
    // Get the global threading proxy instance for Swift
    ThreadingProxy* getThreadingProxy() {
        return g_threadingProxy;
    }
    
    std::vector<RenderCommand> getAndClearCommandsFromProxy() {
        if (g_threadingProxy) {
            return g_threadingProxy->getAndClearCommands();
        }
        return std::vector<RenderCommand>();
    }

} // namespace FloppyTurd