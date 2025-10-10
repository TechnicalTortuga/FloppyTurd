#pragma once

#include "../Core/GNLog.h"
#include <chrono>
#include <string>
#include <unordered_map>

namespace Gnosis {

    // Simple frame profiler - tracks section times without RAII complexity
    class FrameProfiler {
    public:
        explicit FrameProfiler(const std::string& context);

        // Frame lifecycle
        void BeginFrame();
        void EndFrame();
        
        // Section timing - call these in pairs
        void StartSection(const char* name);
        void EndSection(const char* name);
        
        // Enable/disable profiling
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

    private:
        struct SectionStats {
            double totalMs = 0.0;
            double maxMs = 0.0;
            int callCount = 0;
        };

        std::string m_context;
        bool m_enabled;
        int m_frameCount;
        std::chrono::steady_clock::time_point m_sectionStartTime;
        std::chrono::steady_clock::time_point m_lastFlushTime;
        std::unordered_map<std::string, SectionStats> m_stats;
    };

} // namespace Gnosis
