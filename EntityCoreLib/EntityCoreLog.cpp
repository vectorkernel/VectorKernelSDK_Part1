#include "pch.h"
#include "EntityCoreLog.h"

#include <atomic>

namespace EntityCore
{
    static std::atomic<LogSink> g_sink{ nullptr };

    void SetLogSink(LogSink sink) noexcept
    {
        g_sink.store(sink, std::memory_order_release);
    }

    LogSink GetLogSink() noexcept
    {
        return g_sink.load(std::memory_order_acquire);
    }
}
