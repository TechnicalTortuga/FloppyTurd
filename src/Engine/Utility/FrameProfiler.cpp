#include "FrameProfiler.h"
#include <algorithm>
#include <string>

namespace Gnosis {

    FrameProfiler::FrameProfiler(const std::string& context)
        : m_context(context)
        , m_enabled(false)
        , m_frameCount(0)
        , m_sectionStartTime(std::chrono::steady_clock::now())
        , m_lastFlushTime(std::chrono::steady_clock::now()) {}

    void FrameProfiler::BeginFrame() {
        if (!m_enabled) {
            return;
        }
        ++m_frameCount;
        
        // Debug: Log on first frame to confirm profiler is active
        if (m_frameCount == 1) {
            GN_LOG_INFO("[FrameProfiler] " + m_context + " profiler is active and collecting data");
        }
    }

    void FrameProfiler::EndFrame() {
        if (!m_enabled) {
            return;
        }
        
        // Flush every ~1 second
        auto now = std::chrono::steady_clock::now();
        double secondsSinceFlush = std::chrono::duration<double>(now - m_lastFlushTime).count();
        if (secondsSinceFlush >= 1.0) {
            // Build profiler output using std::string (matches codebase pattern)
            std::string output = "[FrameProfiler] " + m_context + ": frames=" + std::to_string(m_frameCount);
            
            // Output each section's stats
            for (const auto& [name, stats] : m_stats) {
                double avgMs = stats.callCount > 0 ? stats.totalMs / stats.callCount : 0.0;
                output += " | " + std::string(name) +
                         " avg=" + std::to_string(avgMs).substr(0, 5) + "ms" +
                         " max=" + std::to_string(stats.maxMs).substr(0, 5) + "ms" +
                         " calls=" + std::to_string(stats.callCount);
            }
            
            GN_LOG_INFO(output);
            
            // Reset for next interval
            m_stats.clear();
            m_frameCount = 0;
            m_lastFlushTime = now;
        }
    }

    void FrameProfiler::StartSection(const char* name) {
        if (!m_enabled) {
            return;
        }
        m_sectionStartTime = std::chrono::high_resolution_clock::now();
    }

    void FrameProfiler::EndSection(const char* name) {
        if (!m_enabled) {
            return;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double durationMs = std::chrono::duration<double, std::milli>(endTime - m_sectionStartTime).count();
        
        SectionStats& stats = m_stats[name];
        stats.totalMs += durationMs;
        stats.maxMs = std::max(stats.maxMs, durationMs);
        ++stats.callCount;
    }

} // namespace Gnosis
