#include "vxpch.h"
#include "UUID.h"

#include <sstream>
#include <iomanip>

namespace Vortex
{
    std::random_device UUID::s_RandomDevice;
    std::mt19937_64 UUID::s_Generator(s_RandomDevice());
    const UUID UUID::Invalid(0);

    UUID::UUID()
        : m_UUID(Generate().m_UUID)
    {
    }

    UUID::UUID(uint64_t id)
        : m_UUID(id)
    {
    }

    std::string UUID::ToString() const
    {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << m_UUID;
        return ss.str();
    }

    UUID UUID::Generate()
    {
        static std::uniform_int_distribution<uint64_t> distribution;
        return UUID(distribution(s_Generator));
    }

} // namespace Vortex