#pragma once
#include <string>

enum class EntityBookErrc
{
    InvalidCapacity,
    AllocationFailed,
    InitFailed,
    OutOfMemory,
    DuplicateId,
    CapacityExceeded
};

struct EntityBookError
{
    EntityBookErrc code{};
    std::string message;
};
