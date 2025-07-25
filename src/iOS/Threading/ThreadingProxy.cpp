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
        cmd.data.clearScreen.r = r;
        cmd.data.clearScreen.g = g;
        cmd.data.clearScreen.b = b;
        cmd.data.clearScreen.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawSprite(void* sprite, float x, float y, float rotation) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_SPRITE;
        cmd.data.drawSprite.sprite = sprite;
        cmd.data.drawSprite.x = x;
        cmd.data.drawSprite.y = y;
        cmd.data.drawSprite.rotation = rotation;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawSpriteScaled(void* sprite, float x, float y, float scaleX, float scaleY, float rotation) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_SPRITE_SCALED;
        cmd.data.drawSpriteScaled.sprite = sprite;
        cmd.data.drawSpriteScaled.x = x;
        cmd.data.drawSpriteScaled.y = y;
        cmd.data.drawSpriteScaled.scaleX = scaleX;
        cmd.data.drawSpriteScaled.scaleY = scaleY;
        cmd.data.drawSpriteScaled.rotation = rotation;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawText(const char* text, float x, float y, float fontSize, float r, float g, float b, float a) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_TEXT;
        cmd.data.drawText.text = text;  // Note: Caller must ensure string lifetime
        cmd.data.drawText.x = x;
        cmd.data.drawText.y = y;
        cmd.data.drawText.fontSize = fontSize;
        cmd.data.drawText.r = r;
        cmd.data.drawText.g = g;
        cmd.data.drawText.b = b;
        cmd.data.drawText.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_RECTANGLE;
        cmd.data.drawRect.x = x;
        cmd.data.drawRect.y = y;
        cmd.data.drawRect.width = width;
        cmd.data.drawRect.height = height;
        cmd.data.drawRect.r = r;
        cmd.data.drawRect.g = g;
        cmd.data.drawRect.b = b;
        cmd.data.drawRect.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_DRAW_CIRCLE;
        cmd.data.drawCircle.x = x;
        cmd.data.drawCircle.y = y;
        cmd.data.drawCircle.radius = radius;
        cmd.data.drawCircle.r = r;
        cmd.data.drawCircle.g = g;
        cmd.data.drawCircle.b = b;
        cmd.data.drawCircle.a = a;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::enqueueGetScreenSize(float* width, float* height) {
        if (!s_instance) return;
        
        RenderCommand cmd;
        cmd.type = CMD_GET_SCREEN_SIZE;
        cmd.data.getScreenSize.width = width;
        cmd.data.getScreenSize.height = height;
        s_instance->enqueueCommand(cmd);
    }

    void ThreadingProxy::setSwiftComponents(MetalRenderer* renderer, TouchInputHandler* input, AudioManagerSwift* audio) {
        m_metalRenderer = renderer;
        m_touchInputHandler = input;
        m_audioManager = audio;
    }
    
    std::queue<RenderCommand> ThreadingProxy::getAndClearCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::queue<RenderCommand> commands;
        commands.swap(m_commandQueue);
        return commands;
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

} // namespace FloppyTurd