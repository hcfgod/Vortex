#pragma once

#include <cstdint>
#include <string>
#include <random>

namespace Vortex
{
    /**
     * @brief Simple UUID implementation for engine objects
     * 
     * Uses a 64-bit integer for simplicity and performance.
     * For a full engine, you might want to use a proper UUID library.
     */
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t id);
        UUID(const UUID&) = default;
        UUID& operator=(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }
        uint64_t GetValue() const { return m_UUID; }

        std::string ToString() const;

        bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }
        bool operator!=(const UUID& other) const { return m_UUID != other.m_UUID; }
        bool operator<(const UUID& other) const { return m_UUID < other.m_UUID; }

        static UUID Generate();
        static const UUID Invalid;

    private:
        uint64_t m_UUID;
        static std::random_device s_RandomDevice;
        static std::mt19937_64 s_Generator;
    };

} // namespace Vortex

namespace std
{
    template<>
    struct hash<Vortex::UUID>
    {
        size_t operator()(const Vortex::UUID& uuid) const
        {
            return hash<uint64_t>{}(uuid.GetValue());
        }
    };
}