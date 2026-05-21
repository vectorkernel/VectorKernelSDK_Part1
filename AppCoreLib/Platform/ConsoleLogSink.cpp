#include "pch.h"
#include "ConsoleLogSink.h"

#include <iostream>

namespace AppCore
{
    void ConsoleLogSink(EntityCore::LogLevel level,
        const char* category,
        const char* event,
        std::string_view message)
    {
        const char* lvl = "TRACE";
        switch (level)
        {
        case EntityCore::LogLevel::Trace: lvl = "TRACE"; break;
        case EntityCore::LogLevel::Info:  lvl = "INFO "; break;
        case EntityCore::LogLevel::Warn:  lvl = "WARN "; break;
        case EntityCore::LogLevel::Error: lvl = "ERROR"; break;
        default: break;
        }

        const char* cat = category ? category : "(null)";
        const char* evt = event ? event : "(null)";

        std::cout << "[" << lvl << "] " << cat << "::" << evt;
        if (!message.empty())
            std::cout << " | " << message;
        std::cout << "\n";
    }

    void InstallConsoleLogSink() noexcept
    {
        EntityCore::SetLogSink(&ConsoleLogSink);
    }
}
