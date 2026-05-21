#pragma once
#include <cstddef>
#include "VklExpected.h"
#include <memory>

struct EntityBookError;   // forward declare
class EntityBook;         // forward declare

vkl::expected<std::unique_ptr<EntityBook>, EntityBookError>
CreateEntityBook(std::size_t capacity);