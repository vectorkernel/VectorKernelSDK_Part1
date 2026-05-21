#include <exception>
#include <iostream>
#include <memory>

#include "../EntityCoreLib/EntityBook.h"
#include "../EntityCoreLib/EntityBookFactory.h"
#include "../EntityCoreLib/LineEntity.h"

namespace
{
    constexpr std::size_t kStartCapacity = 10000;

    std::unique_ptr<LineEntity> MakeLine(std::size_t id)
    {
        auto line = std::make_unique<LineEntity>();
        line->ID = id;
        line->start = glm::vec3(static_cast<float>(id), 0.0f, 0.0f);
        line->end   = glm::vec3(static_cast<float>(id), 10.0f, 0.0f);
        line->color = glm::vec4(1.0f);
        line->width = 1.0f;
        return line;
    }
}

int main()
{
    try
    {
        std::cout << "Example0005 - EntityBook Overflow\\n";
        std::cout << "================================\\n\\n";

        auto finalBookResult = CreateEntityBook(kStartCapacity);
        if (!finalBookResult.has_value())
        {
            std::cout << "Failed to create the initial EntityBook: "
                      << finalBookResult.error().message << "\\n";
            return 1;
        }

        auto book = std::move(finalBookResult.value());
        std::cout << "Created EntityBook with initial capacity " << book->Capacity() << "\\n";
        std::cout << "Filling EntityBook. The book will attempt to grow automatically when full...\\n";

        std::size_t inserted = 0;
        std::size_t lastReportedCapacity = book->Capacity();
        for (;;)
        {
            book->AddEntity(MakeLine(inserted + 1));
            ++inserted;

            if (book->Capacity() != lastReportedCapacity)
            {
                std::cout << "EntityBook auto-resized from " << lastReportedCapacity
                          << " to " << book->Capacity() << " entities\\n";
                lastReportedCapacity = book->Capacity();
            }

            if ((inserted % 10000) == 0)
            {
                std::cout << "Inserted entities: " << inserted
                          << " | Current capacity: " << book->Capacity() << "\\n";
            }
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "\\nALERT: There is no more available space in the EntityBook.\\n";
        std::cerr << "Reason: " << ex.what() << "\\n";
        std::cerr << "The drawing is too large for available memory.\\n";
        std::cerr << "The application will now exit gracefully.\\n";
        return 1;
    }
}
