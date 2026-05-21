#pragma once

#include <utility>
#include <variant>
#include <type_traits>
#include <stdexcept>

namespace vkl
{
    template <typename E>
    class unexpected
    {
    public:
        unexpected(const E& error) : m_error(error) {}
        unexpected(E&& error) : m_error(std::move(error)) {}

        const E& error() const& noexcept { return m_error; }
        E& error() & noexcept { return m_error; }
        E&& error() && noexcept { return std::move(m_error); }

    private:
        E m_error;
    };

    template <typename T, typename E>
    class expected
    {
    public:
        expected(const T& value) : m_storage(value) {}
        expected(T&& value) : m_storage(std::move(value)) {}
        expected(const unexpected<E>& error) : m_storage(error.error()) {}
        expected(unexpected<E>&& error) : m_storage(std::move(error.error())) {}

        expected(const expected&) = default;
        expected(expected&&) noexcept = default;
        expected& operator=(const expected&) = default;
        expected& operator=(expected&&) noexcept = default;
        ~expected() = default;

        [[nodiscard]] bool has_value() const noexcept
        {
            return std::holds_alternative<T>(m_storage);
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return has_value();
        }

        T& value() &
        {
            if (!has_value())
            {
                throw std::logic_error("bad expected access");
            }
            return std::get<T>(m_storage);
        }

        const T& value() const&
        {
            if (!has_value())
            {
                throw std::logic_error("bad expected access");
            }
            return std::get<T>(m_storage);
        }

        T&& value() &&
        {
            if (!has_value())
            {
                throw std::logic_error("bad expected access");
            }
            return std::move(std::get<T>(m_storage));
        }

        E& error() &
        {
            return std::get<E>(m_storage);
        }

        const E& error() const&
        {
            return std::get<E>(m_storage);
        }

        E&& error() &&
        {
            return std::move(std::get<E>(m_storage));
        }

    private:
        std::variant<T, E> m_storage;
    };
}
