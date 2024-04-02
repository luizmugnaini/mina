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
/// Description: Base header for the Mina emulator source code.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <type_traits>

namespace mina {
    // -------------------------------------------------------------------------------------------------
    // Concepts.
    // -------------------------------------------------------------------------------------------------

    template <typename T>
    concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

    template <typename T>
    concept NotTriviallyCopyable = !std::is_trivially_copyable_v<T>;

    template <typename T>
    concept Integral = std::is_integral_v<T>;

    template <typename T>
    concept Numeric = std::is_integral_v<T> || std::is_floating_point_v<T>;

    template <typename T>
    concept Pointer = std::is_pointer_v<T>;

    template <typename T>
    concept NotPointer = !std::is_pointer_v<T>;

    template <typename T>
    concept Addable = requires(T x) { x + x; };

    template <typename T>
    concept Reflexive = requires(T x, T y) { x == y; };

    template <typename T>
    concept PartiallyOrdered = requires(T x, T y) {
        x <= y;
        x == y;
    };

    // -------------------------------------------------------------------------------------------------
    // Internal type definitions.
    // -------------------------------------------------------------------------------------------------

    /// Unsigned integer type.
    using u8    = std::uint8_t;
    using u16   = std::uint16_t;
    using u32   = std::uint32_t;
    using u64   = std::uint64_t;
    using usize = u64;

    /// Signed integer type.
    using i8    = std::int8_t;
    using i16   = std::int16_t;
    using i32   = std::int32_t;
    using i64   = std::int64_t;
    using isize = i64;

    /// Memory-address types.
    using uptr    = u64;
    using iptr    = i64;
    using BusAddr = u16;

    /// Floating-point types.
    using f32 = float;
    using f64 = double;

    /// Immutable zero-terminated string type
    ///
    /// A pointer to a contiguous array of constant character values.
    using StrPtr = char const*;

    /// A string literal type.
    ///
    /// This type is used to enforce the passing of literal strings to formatting functions such as
    /// `mina::log_fmt`.
    struct StringLiteral {
        StrPtr const str;
        usize const  len;

        template <usize N>
        consteval StringLiteral(char const (&_str)[N]) : str{_str}, len{N} {}

        StringLiteral(StringLiteral&)       = delete;
        StringLiteral(StringLiteral const&) = delete;
    };

/// Simple macro casting an unused result from a function call.
#define mina_unused(x) (void)x

    // -------------------------------------------------------------------------------------------------
    // Logging functionality of the engine.
    //
    // All logging functions should be accessed mostly via macros in order to have the caller file
    // name and line seamlessly handled.
    // -------------------------------------------------------------------------------------------------

    /// Log level.
    ///
    /// The logs are set in an increasing level of vebosity, where `LogLevel::Fatal` is the lowest
    /// and `LogLevel::Debug` is the highest.
    enum class LogLevel {
        Fatal   = 0,  ///< Unrecoverable error.
        Error   = 1,  ///< Recoverable error.
        Warning = 2,  ///< Indicates that something non-optimal may have happened.
        Info    = 3,  ///< General message to state any useful information.
        Debug   = 4,  ///< Serves only for debugging purposes in development.
    };

    constexpr StrPtr log_level_str(LogLevel level) {
        switch (level) {
            case LogLevel::Fatal:
                return "\x1b[1;41m[FATAL]\x1b[0m";
            case LogLevel::Error:
                return "\x1b[1;31m[ERROR]\x1b[0m";
            case LogLevel::Warning:
                return "\x1b[1;33m[WARNING]\x1b[0m";
            case LogLevel::Info:
                return "\x1b[1;32m[INFO]\x1b[0m";
            case LogLevel::Debug:
                return "\x1b[1;34m[DEBUG]\x1b[0m";
        }
    }

    /// Write a message to the standard error stream.
    ///
    /// Parameters:
    ///     * file: Name of the source file of the caller.
    ///     * line: Current line number of the source file of the caller.
    ///     * level: The level associated to the log message.
    ///     * msg: Log message to be displayed.
    inline void log_(std::source_location&& loc, LogLevel lvl, StrPtr msg) noexcept {
#if !defined(MINA_DISABLE_LOGGING)
        (void)std::fprintf(
            stderr,
            "%s [%s:%d] %s\n",
            log_level_str(lvl),
            loc.file_name(),
            loc.line(),
            msg);
#endif
    }

    template <typename... Arg>
    void log_fmt_(
        std::source_location const& loc,
        LogLevel                    lvl,
        StringLiteral&&             fmt,
        Arg const&... args) noexcept {
#if !defined(MINA_DISABLE_LOGGING)
        constexpr usize max_msg_size = 8192;

        // Format the string with the given arguments.
        char      msg[max_msg_size];
        i32 const char_count = std::snprintf(msg, max_msg_size - 1, fmt.str, args...);
        assert(char_count >= 0 && "std::snptrintf unable to parse the format string and arguments");

        // Stamp the message with a null-terminator.
        usize const msg_size =
            char_count < max_msg_size ? static_cast<usize>(char_count) : max_msg_size;
        msg[msg_size] = 0;

        (void)std::fprintf(
            stderr,
            "%s [%s:%d] %s\n",
            log_level_str(lvl),
            loc.file_name(),
            loc.line(),
            msg);
#endif
    }

    // clang-format off
    // {

#define mina_log(lvl, msg)           mina::log_(std::source_location::current(), lvl, msg)
#define mina_log_fmt(lvl, fmt, ...)  mina::log_fmt_(std::source_location::current(), lvl, fmt, __VA_ARGS__)

    // }
    // clang-format on

    // -------------------------------------------------------------------------------------------------
    // Main functionality for writing tests and assertions.
    //
    // In order to disable assertions you can compile with the flag `-DMINA_DISABLE_ASSERTS`. This
    // will make assertions evaluate the given expression and throw away its result.
    // -------------------------------------------------------------------------------------------------

    inline void
    assert_(std::source_location&& src_loc, bool expr_res, StrPtr expr_str, StrPtr msg = "") {
#if !defined(MINA_DISABLE_ASSERTS)
        if (!expr_res) {
            log_fmt_(src_loc, LogLevel::Fatal, "Assertion failed: %s, msg: %s", expr_str, msg);
            std::abort();
        }
#endif
    }

    // clang-format off
    // {

#define mina_assert(expr)          mina::assert_(std::source_location::current(), expr, #expr)
#define mina_assert_msg(expr, msg) mina::assert_(std::source_location::current(), expr, #expr, msg)
#define mina_unreachable()       { mina::log_fmt(std::source_location::current(), mina::LogLevel::Fatal, "Codepath should be unreachable!"); std::abort(); }

    // }
    // clang-format on

    // -------------------------------------------------------------------------------------------------
    // Utility types.
    // -------------------------------------------------------------------------------------------------

    /// Option type.
    template <typename T>
        requires TriviallyCopyable<T>
    struct Option {
        T const    val{};
        bool const has_val = false;

        constexpr Option() = default;

        explicit constexpr Option(T _val)
            requires NotPointer<T>
            : val{_val}, has_val{true} {}

        constexpr Option(T _val)
            requires Pointer<T>
            : val{_val}, has_val{_val != nullptr} {}

        [[nodiscard]] constexpr bool is_some() const noexcept {
            return has_val;
        }
        [[nodiscard]] constexpr bool is_none() const noexcept {
            return !has_val;
        }

        [[nodiscard]] constexpr T val_or(T default_val = T{}) const noexcept {
            return has_val ? val : default_val;
        }
        [[nodiscard]] T demand(StrPtr msg = "Option::expect_val failed") const noexcept {
            mina_assert_msg(has_val, msg);
            return val;
        }
    };

    /// Type holding an immutable pointer that can never be null.
    template <typename T>
        requires NotPointer<T>
    struct NotNull {
        T* const ptr;

        /// Create a non-null pointer wrapper.
        ///
        /// Parameters:
        ///     * `ptr`: Non-null pointer.
        ///     * `msg`: Failure message in case `ptr` fails to be non-null.
        NotNull(T* _ptr, StrPtr msg = "NotNull created with a null pointer") noexcept : ptr{_ptr} {
            mina_assert_msg(ptr != nullptr, msg);
        }
    };

    /// Fat pointer.
    template <typename T>
    struct FatPtr {
        T*    buf  = nullptr;
        usize size = 0;

        explicit constexpr FatPtr() noexcept = default;

        explicit FatPtr(T* _buf, usize _length) noexcept : buf{_buf}, size{_length} {
            if (size != 0) {
                mina_assert_msg(
                    buf != nullptr,
                    "FatPtr constructed with inconsistent data: non-zero length but null buffer");
            }
        }

        static constexpr FatPtr<u8> as_bytes(auto* buf, usize length) noexcept {
            return FatPtr<u8>{reinterpret_cast<u8*>(buf), sizeof(decltype(*buf)) * length};
        }

        [[nodiscard]] constexpr usize size_bytes() const noexcept {
            return sizeof(T) * size;
        }

        [[nodiscard]] constexpr T* begin() const noexcept {
            return buf;
        }
        [[nodiscard]] constexpr T* end() const noexcept {
            return (buf == nullptr) ? nullptr
                                    : reinterpret_cast<T*>(reinterpret_cast<uptr>(buf) + size);
        }
    };
}  // namespace mina
