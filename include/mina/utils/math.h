///                          Mina, Game Boy emulator
///    Copyright (C) 2024 Luiz Gustavo Mugnaini Anselmo
///
///    This program is free software; you can redistribute it and/or modify
///    it under the terms of the GNU General Public License as published by
///    the Free Software Foundation; either version 2 of the License, or
///    (at your option) any later version.
///
///    This program is distributed in the hope that it will be useful,
///    but WITHOUT ANY WARRANTY; without even the implied warranty of
///    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///    GNU General Public License for more details.
///
///    You should have received a copy of the GNU General Public License along
///    with this program; if not, write to the Free Software Foundation, Inc.,
///    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
///
///
/// Description: Common mathematical utilities.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/base.h>

#include <limits>

namespace mina {
    /// Minimum between two elements that admit a partial order.
    ///
    /// Return: `lhs` if `lhs` is less than or equal to `rhs`, otherwise returns `rhs`.
    template <typename T>
        requires PartiallyOrdered<T> && TriviallyCopyable<T>
    constexpr T min(T lhs, T rhs) noexcept {
        return (lhs <= rhs) ? lhs : rhs;
    }

    /// Specialized minimum value of unsigned integers without branching.
    template <typename T>
        requires std::is_unsigned_v<T> && std::is_integral_v<T>
    constexpr T min(T lhs, T rhs) noexcept {
        return rhs ^ ((lhs ^ rhs) & -(lhs < rhs));
    }

    /// Maximum between two elements that admit a partial order.
    ///
    /// Return: `lhs` if `lhs` is greater than or equal to `rhs`, otherwise returns `rhs`.
    template <typename T>
        requires PartiallyOrdered<T> && TriviallyCopyable<T>
    constexpr T max(T lhs, T rhs) noexcept {
        return (lhs >= rhs) ? lhs : rhs;
    }

    /// Specialized maximum value of unsigned integers without branching.
    template <typename T>
        requires std::is_unsigned_v<T> && Integral<T>
    constexpr T max(T lhs, T rhs) noexcept {
        return lhs ^ ((lhs ^ rhs) & -(lhs < rhs));
    }

    /// Checks if a given number is a power of two.
    template <typename T>
        requires Integral<T> && TriviallyCopyable<T>
    constexpr bool is_power_of_two(T n) noexcept {
        return (n > 0) && !(n & (n - 1));
    }

    /// Add with a lower-bound.
    template <typename T>
        requires Addable<T> && PartiallyOrdered<T> && TriviallyCopyable<T>
    constexpr T lb_add(T a, T b, T lower_bound) noexcept {
        T const c = a + b;
        return (c <= lower_bound) ? lower_bound : c;
    }

    /// Add with an upper-bound.
    template <typename T>
        requires Addable<T> && PartiallyOrdered<T> && TriviallyCopyable<T>
    constexpr T ub_add(T a, T b, T upper_bound) noexcept {
        T const c = a + b;
        return (c >= upper_bound) ? upper_bound : c;
    }

    /// Add two values wrapping the result to the corresponding maximal numeric limit.
    template <typename T>
        requires Addable<T> && std::is_unsigned_v<T> && TriviallyCopyable<T>
    constexpr T wrap_add(T a, T b) noexcept {
        T const c = a + b;
        return (c >= a) ? c : std::numeric_limits<T>::max();
    }

    /// Subtract two values wrapping the result to the corresponding minimal numeric limit.
    template <typename T>
        requires Addable<T> && std::is_unsigned_v<T> && TriviallyCopyable<T>
    constexpr T wrap_sub(T a, T b) noexcept {
        T const c = a - b;
        return (c <= a) ? c : std::numeric_limits<T>::min();
    }

    /// Clamp a number to a given min-max range.
    template <typename T>
        requires PartiallyOrdered<T> && TriviallyCopyable<T>
    constexpr T clamp(T x, T min, T max) noexcept {
        if (x <= min) {
            return min;
        }
        if (x >= max) {
            return max;
        }
        return x;
    }
}  // namespace mina
