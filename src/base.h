/// Base header for the Mina emulator source code.
///
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>
#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <type_traits>

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
using uptr = u64;
using iptr = i64;

/// Floating-point types.
using f32 = float;
using f64 = double;

/// Immutable zero-terminated string type
///
/// A pointer to a contiguous array of constant character values.
using StrPtr = char const*;

struct StringLiteral {
    StrPtr str;

    template <usize N>
    consteval StringLiteral(char const (&_str)[N]) : str{_str} {}
};

// -------------------------------------------------------------------------------------------------
// Miscellaneous useful macros.
// -------------------------------------------------------------------------------------------------

/// Simple macro casting an unused variable to void.
#define mina_unused_var(x) (void)x

/// Simple macro casting an unused result from a function call.
#define mina_unused_result(x) (void)x

/// Codepath should be unreachable, abort the program.
#define mina_unreachable() \
    { log_fatal_and_exit_va_(__FILE__, __LINE__, "Codepath should be unreachable!\n"); }

// -------------------------------------------------------------------------------------------------
// Logging functionality of the engine.
//
// All logging functions should be accessed mostly via macros in order to have the caller file name
// and line seamlessly handled.
// -------------------------------------------------------------------------------------------------

// TODO: implement a log-level filter.
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
        case LogLevel::Debug:
            return "\x1b[1;34m[DEBUG]\x1b[0m";
        case LogLevel::Info:
            return "\x1b[1;32m[INFO]\x1b[0m";
    }
}

/// Abort the program abnormally.
void abort_program() noexcept;

/// Write a message to the standard error stream.
///
/// Parameters:
///     * file: Name of the source file of the caller.
///     * line: Current line number of the source file of the caller.
///     * level: The level associated to the log message.
///     * msg: Log message to be displayed.
inline void log_msg(StrPtr file, i32 line, LogLevel level, StrPtr msg) noexcept {
    mina_unused_result(
        std::fprintf(stderr, "%s [%s:%d] %s", log_level_str(level), file, line, msg));
}

/// Write formatted string to the standard error stream.
///
/// Parameters:
///     * file: Name of the source file of the caller.
///     * line: Current line number of the source file of the caller.
///     * level: The level associated to the log message.
///     * fmt: The formatting string.
///     * args: Remaining arguments used by the `fmt` string.
template <typename... Arg>
void log_fmt(
    StrPtr        file,
    i32           line,
    LogLevel      level,
    StringLiteral fmt,
    Arg const&... args) noexcept {
    constexpr usize max_msg_size = 8192;

    char      msg[max_msg_size];
    i32 const char_count = std::snprintf(msg, max_msg_size, fmt.str, args...);
    assert(char_count != -1 && "std::snptrintf unable to parse the format string and arguments");

    if (char_count < static_cast<isize>(max_msg_size)) {
        // The whole image fits on the buffer.
        msg[char_count + 1] = 0;
    } else {
        // Unable to fit the whole message to the buffer.
        msg[max_msg_size - 1] = 0;
    }

    mina_unused_result(
        std::fprintf(stderr, "%s [%s:%d] %s", log_level_str(level), file, line, msg));
}

// clang-format off
#if defined(MINA_DISABLE_LOGGING)
#    define log_fatal(msg)          0
#    define log_fatal_and_exit(msg) 0
#    define log_error(msg)          0
#    define log_warning(msg)        0
#    define log_debug(msg)          0
#    define log_info(msg)           0

#    define log_fatal_va(msg, ...)          0
#    define log_fatal_and_exit_va(msg, ...) 0
#    define log_error_va(msg, ...)          0
#    define log_warning_va(msg, ...)        0
#    define log_debug_va(msg, ...)          0
#    define log_info_va(msg, ...)           0
#else

/// Write a fatal error message to the standard error stream.
#    define log_fatal(msg) log_msg(__FILE__, __LINE__, LogLevel::Fatal, msg)


/// Write a fatal error to the standard error stream and quit the program.
#    define log_fatal_and_exit(msg) { log_msg(__FILE__, __LINE__, LogLevel::Fatal, msg); assert(false && "Fatal error, aborting..."); }

/// Write an error message to the standard error stream.
#    define log_error(msg) log_msg(__FILE__, __LINE__, LogLevel::Error, msg)

/// Write a warning message to the standard error stream.
#    define log_warning(msg) log_msg(__FILE__, __LINE__, LogLevel::Warning, msg)


/// Write a debug message to the standard error stream.
#    define log_debug(msg) log_msg(__FILE__, __LINE__, LogLevel::Debug, msg)


/// Write an useful info message to the standard error stream.
#    define log_info(msg) log_msg(__FILE__, __LINE__, LogLevel::Info, msg)

/// Write a fatal error message to the standard error stream with variadic arguments.
#    define log_fatal_va(msg, ...) log_fmt(__FILE__, __LINE__, LogLevel::Fatal, msg, __VA_ARGS__)

/// Write a fatal error message to the standard error stream with variadic arguments, and exit the program.
#    define log_fatal_and_exit_va(msg, ...) { log_fmt(__FILE__, __LINE__, msg, __VA_ARGS__); assert(false && "Fatal error, aborting..."); }

/// Write an error message to the standard error stream with variadic arguments.
#    define log_error_va(msg, ...) log_fmt(__FILE__, __LINE__, LogLevel::Error, msg, __VA_ARGS__)

/// Write a warning message to the standard error stream with variadic arguments.
#    define log_warning_va(msg, ...) log_fmt(__FILE__, __LINE__, LogLevel::Warning, msg, __VA_ARGS__)

/// Write a debug message to the standard error stream with variadic arguments.
#    define log_debug_va(msg, ...) log_fmt(__FILE__, __LINE__, LogLevel::Debug, msg, __VA_ARGS__)


/// Write an useful info message to the standard error stream with variadic arguments.
#    define log_info_va(msg, ...) log_fmt(__FILE__, __LINE__, LogLevel::Info, msg, __VA_ARGS__)
#endif  // MINA_DISABLE_LOGGING
// clang-format on

// -------------------------------------------------------------------------------------------------
// Main functionality for writing tests and assertions.
//
// In order to disable assertions you can compile with the flag `-DMINA_DISABLE_ASSERTS`. This will
// make assertions no-op even though you may have them in your code.
// -------------------------------------------------------------------------------------------------

// clang-format off
#if defined(MINA_DISABLE_ASSERTS)
#    define mina_assert(expr)          ((void)expr)
#    define mina_assert_msg(expr, msg) ((void)expr)
#else
#    define mina_assert(expr) { if (!expr) { log_msg(__FILE__, __LINE__, LogLevel::Fatal, "Assertion failed: %s", #expr); assert(false && "Fatal error, aborting..."); } }
#    define mina_assert_msg(expr, msg) { if (!expr) { log_fmt(__FILE__, __LINE__, LogLevel::Fatal, "Assertion failed: %s, message: %s", #expr, msg); assert(false && "Fatal error, aborting..."); } }
#endif  // MINA_DISABLE_ASSERTS
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

    constexpr T& operator*() noexcept {
        return *ptr;
    }
    constexpr T* operator->() noexcept {
        return ptr;
    }

    NotNull() = delete;
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

constexpr FatPtr<u8> fat_ptr_as_bytes(auto* buf, usize length) noexcept {
    return FatPtr<u8>{reinterpret_cast<u8*>(buf), sizeof(decltype(*buf)) * length};
}
