#pragma once

#include <type_traits>
#include <functional>
#include <utility>
#include "Core/Debug/Assert.h"

namespace Vortex
{
    /**
     * SmartRef<T> is a tiny, non-owning reference wrapper.
     *
     * Design goals:
     * - Lightweight handle to an existing object (no ownership)
     * - Default-constructible and rebindable (unlike T&)
     * - Familiar pointer-like access (operator->, operator*)
     * - Implicit conversion to T& for APIs that expect references
     * - Debug assertions on null access
     *
     * Notes:
     * - This is not thread-safe and does not perform lifetime tracking.
     *   Ensure the referenced object outlives the SmartRef usage.
     * - To allow const usage, instantiate as SmartRef<const T>.
     */
    template <typename T>
    class SmartRef
    {
    public:
        using element_type = T;

        // Constructors
        constexpr SmartRef() noexcept
            : m_Ptr(nullptr) {}

        constexpr SmartRef(T& ref) noexcept
            : m_Ptr(std::addressof(ref)) {}

        constexpr SmartRef(T* ptr) noexcept
            : m_Ptr(ptr) {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        constexpr SmartRef(std::reference_wrapper<U> ref) noexcept
            : m_Ptr(std::addressof(ref.get())) {}

        // Default copy/move
        constexpr SmartRef(const SmartRef&) noexcept = default;
        constexpr SmartRef& operator=(const SmartRef&) noexcept = default;
        constexpr SmartRef(SmartRef&&) noexcept = default;
        constexpr SmartRef& operator=(SmartRef&&) noexcept = default;

        // Observers
        constexpr T* ptr() const noexcept { return m_Ptr; }
        constexpr explicit operator bool() const noexcept { return m_Ptr != nullptr; }

        // Pointer-like access with assertions
        constexpr T& operator*() const
        {
            VX_CORE_ASSERT(m_Ptr != nullptr, "Dereferencing null SmartRef");
            return *m_Ptr;
        }

        constexpr T* operator->() const
        {
            VX_CORE_ASSERT(m_Ptr != nullptr, "Accessing null SmartRef");
            return m_Ptr;
        }

        // Implicit conversion to reference for seamless interop
        constexpr operator T&() const
        {
            VX_CORE_ASSERT(m_Ptr != nullptr, "Converting null SmartRef to reference");
            return *m_Ptr;
        }

        // Named accessors
        constexpr T& get() const { return static_cast<T&>(*this); }

        // Modifiers
        constexpr void Reset() noexcept { m_Ptr = nullptr; }
        constexpr void Reset(T& ref) noexcept { m_Ptr = std::addressof(ref); }
        constexpr void Reset(T* ptr) noexcept { m_Ptr = ptr; }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        constexpr void Reset(std::reference_wrapper<U> ref) noexcept { m_Ptr = std::addressof(ref.get()); }

        // Comparisons
        constexpr bool operator==(std::nullptr_t) const noexcept { return m_Ptr == nullptr; }
        constexpr bool operator!=(std::nullptr_t) const noexcept { return m_Ptr != nullptr; }

        template <typename U>
        constexpr bool operator==(const SmartRef<U>& other) const noexcept { return m_Ptr == other.ptr(); }

        template <typename U>
        constexpr bool operator!=(const SmartRef<U>& other) const noexcept { return m_Ptr != other.ptr(); }

    private:
        T* m_Ptr;
    };

    // Helper factories (mirroring std::ref/cref naming)
    template <typename T>
    constexpr SmartRef<T> Ref(T& t) noexcept { return SmartRef<T>(t); }

    template <typename T>
    constexpr SmartRef<const T> CRef(const T& t) noexcept { return SmartRef<const T>(t); }
}

