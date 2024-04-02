/// Function utilities for memory-related operations.
///
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/base.h>

namespace mina {
    constexpr i32 bit(i32 n) noexcept {
        return 1 << n;
    }

    constexpr u8 addr_middle_byte(BusAddr addr) noexcept {
        return (addr >> 4) & 0xff;
    }

    template <typename T>
    using MatchFn = bool(T lhs, T rhs);

    /// Check if a range given by a fat pointer contains a given `match` element.
    template <typename T>
        requires std::is_object_v<T> && TriviallyCopyable<T>
    bool contains(T match, FatPtr<T> const& container, NotNull<MatchFn<T>> match_fn) {
        for (auto const& m : container) {
            if (match_fn.ptr(match, m)) {
                return true;
            }
        }
        return false;
    }

    /// Check if a range given by a fat pointer contains a given `match` element.
    template <typename T>
        requires std::is_object_v<T> && TriviallyCopyable<T> && Reflexive<T>
    bool contains(T match, FatPtr<T> const& container) {
        for (auto const& m : container) {
            if (match == m) {
                return true;
            }
        }
        return false;
    }

    /// Add an offset to a pointer.
    template <typename T>
    constexpr T* ptr_add(T const* ptr, uptr offset) {
        return (ptr == nullptr) ? nullptr
                                : reinterpret_cast<T*>(reinterpret_cast<uptr>(ptr) + offset);
    }

    /// Subtract an offset from a pointer.
    template <typename T>
    constexpr T* ptr_sub(T const* ptr, uptr offset) {
        return (ptr == nullptr)
                   ? nullptr
                   : reinterpret_cast<T*>(reinterpret_cast<iptr>(ptr) - static_cast<iptr>(offset));
    }

    /// Compute the size of a contiguous array of instances of a given type.
    template <typename T>
        requires std::is_object_v<T>
    constexpr usize array_size(usize length) noexcept {
        return length * sizeof(T);
    }

    /// Simple wrapper around `std::memset` that automatically deals with null values.
    ///
    /// Does nothing if `ptr` is a null pointer.
    void memory_set(FatPtr<u8> fat_ptr, i32 fill) noexcept;

    /// Given a pointer to a structure, zeroes every field of said instance.
    template <typename T>
        requires NotPointer<T>
    void zero_struct(T* obj) noexcept {
        memory_set(fat_ptr_as_bytes(obj, 1), 0);
    }

    /// Simple wrapper around `std::memcpy`.
    ///
    /// This function will assert that the blocks of memory don't overlap, avoiding undefined
    /// behaviour introduced by `std::memcpy` in this case.
    void memory_copy(u8* dest, u8 const* src, usize size) noexcept;

    /// Simple wrapper around `std::memmove`.
    ///
    /// Does nothing if either `dest` or `src` are null pointers.
    void memory_move(u8* dest, u8 const* src, usize size) noexcept;
}  // namespace mina
