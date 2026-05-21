#pragma once

#include <string_view>

#include "..\..\EntityCoreLib\EntityCoreLog.h"

namespace AppCore
{
    void ConsoleLogSink(EntityCore::LogLevel level,
        const char* category,
        const char* event,
        std::string_view message);

    void InstallConsoleLogSink() noexcept;
}
