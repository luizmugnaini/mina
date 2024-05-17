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
/// Description: Window handling layer.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <psh/types.h>

#if !defined(GLFW_INCLUDE_NONE)
#    define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace mina::gfx {
    enum Key {
        Space        = GLFW_KEY_SPACE,
        Apostrophe   = GLFW_KEY_APOSTROPHE,
        Comma        = GLFW_KEY_COMMA,
        Minus        = GLFW_KEY_MINUS,
        Period       = GLFW_KEY_PERIOD,
        Slash        = GLFW_KEY_SLASH,
        Zero         = GLFW_KEY_0,
        One          = GLFW_KEY_1,
        Two          = GLFW_KEY_2,
        Three        = GLFW_KEY_3,
        Four         = GLFW_KEY_4,
        Five         = GLFW_KEY_5,
        Six          = GLFW_KEY_6,
        Seven        = GLFW_KEY_7,
        Eight        = GLFW_KEY_8,
        Nine         = GLFW_KEY_9,
        Semicolon    = GLFW_KEY_SEMICOLON,
        Equal        = GLFW_KEY_EQUAL,
        A            = GLFW_KEY_A,
        B            = GLFW_KEY_B,
        C            = GLFW_KEY_C,
        D            = GLFW_KEY_D,
        E            = GLFW_KEY_E,
        F            = GLFW_KEY_F,
        G            = GLFW_KEY_G,
        H            = GLFW_KEY_H,
        I            = GLFW_KEY_I,
        J            = GLFW_KEY_J,
        K            = GLFW_KEY_K,
        L            = GLFW_KEY_L,
        M            = GLFW_KEY_M,
        N            = GLFW_KEY_N,
        O            = GLFW_KEY_O,
        P            = GLFW_KEY_P,
        Q            = GLFW_KEY_Q,
        R            = GLFW_KEY_R,
        S            = GLFW_KEY_S,
        T            = GLFW_KEY_T,
        U            = GLFW_KEY_U,
        V            = GLFW_KEY_V,
        W            = GLFW_KEY_W,
        X            = GLFW_KEY_X,
        Y            = GLFW_KEY_Y,
        Z            = GLFW_KEY_Z,
        LeftBracket  = GLFW_KEY_LEFT_BRACKET,
        Backslash    = GLFW_KEY_BACKSLASH,
        RightBracket = GLFW_KEY_RIGHT_BRACKET,
        GraveAccent  = GLFW_KEY_GRAVE_ACCENT,
        Esc          = GLFW_KEY_ESCAPE,
        Enter        = GLFW_KEY_ENTER,
        Tab          = GLFW_KEY_TAB,
        Backspace    = GLFW_KEY_BACKSPACE,
        Insert       = GLFW_KEY_INSERT,
        Delete       = GLFW_KEY_DELETE,
        Right        = GLFW_KEY_RIGHT,
        Left         = GLFW_KEY_LEFT,
        Down         = GLFW_KEY_DOWN,
        Up           = GLFW_KEY_UP,
        Page_up      = GLFW_KEY_PAGE_UP,
        Page_down    = GLFW_KEY_PAGE_DOWN,
        Home         = GLFW_KEY_HOME,
        End          = GLFW_KEY_END,
        Caps_lock    = GLFW_KEY_CAPS_LOCK,
        Scroll_lock  = GLFW_KEY_SCROLL_LOCK,
        Print_screen = GLFW_KEY_PRINT_SCREEN,
        Pause        = GLFW_KEY_PAUSE,
        F1           = GLFW_KEY_F1,
        F2           = GLFW_KEY_F2,
        F3           = GLFW_KEY_F3,
        F4           = GLFW_KEY_F4,
        F5           = GLFW_KEY_F5,
        F6           = GLFW_KEY_F6,
        F7           = GLFW_KEY_F7,
        F8           = GLFW_KEY_F8,
        F9           = GLFW_KEY_F9,
        F10          = GLFW_KEY_F10,
        F11          = GLFW_KEY_F11,
        F12          = GLFW_KEY_F12,
        F13          = GLFW_KEY_F13,
        F14          = GLFW_KEY_F14,
        F15          = GLFW_KEY_F15,
        F16          = GLFW_KEY_F16,
        F17          = GLFW_KEY_F17,
        F18          = GLFW_KEY_F18,
        F19          = GLFW_KEY_F19,
        F20          = GLFW_KEY_F20,
        F21          = GLFW_KEY_F21,
        F22          = GLFW_KEY_F22,
        F23          = GLFW_KEY_F23,
        F24          = GLFW_KEY_F24,
        F25          = GLFW_KEY_F25,
        KpZero       = GLFW_KEY_KP_0,
        KpOne        = GLFW_KEY_KP_1,
        KpTwo        = GLFW_KEY_KP_2,
        KpThree      = GLFW_KEY_KP_3,
        KpFour       = GLFW_KEY_KP_4,
        KpFive       = GLFW_KEY_KP_5,
        KpSix        = GLFW_KEY_KP_6,
        KpSeven      = GLFW_KEY_KP_7,
        KpEight      = GLFW_KEY_KP_8,
        KpNine       = GLFW_KEY_KP_9,
        KpDecimal    = GLFW_KEY_KP_DECIMAL,
        KpDivide     = GLFW_KEY_KP_DIVIDE,
        KpMultiply   = GLFW_KEY_KP_MULTIPLY,
        KpSubtract   = GLFW_KEY_KP_SUBTRACT,
        KpAdd        = GLFW_KEY_KP_ADD,
        KpEnter      = GLFW_KEY_KP_ENTER,
        KpEqual      = GLFW_KEY_KP_EQUAL,
        LeftShift    = GLFW_KEY_LEFT_SHIFT,
        LeftControl  = GLFW_KEY_LEFT_CONTROL,
        LeftAlt      = GLFW_KEY_LEFT_ALT,
        LeftSuper    = GLFW_KEY_LEFT_SUPER,
        RightShift   = GLFW_KEY_RIGHT_SHIFT,
        RightControl = GLFW_KEY_RIGHT_CONTROL,
        RightAlt     = GLFW_KEY_RIGHT_ALT,
        RightSuper   = GLFW_KEY_RIGHT_SUPER,
        Menu         = GLFW_KEY_MENU,
        Last         = GLFW_KEY_LAST,
    };

    enum struct KeyState {
        Pressed  = GLFW_PRESS,
        Repeat   = GLFW_REPEAT,
        Released = GLFW_RELEASE,
    };

    /// Configuration used to construct a window.
    struct WindowConfig {
        void* user_pointer = nullptr;  ///< User pointer.

        i32 x      = -1;               ///< The starting x position of the window.
        i32 y      = -1;               ///< The starting y position of the window.
        i32 width  = -1;               ///< The starting window width.
        i32 height = -1;               ///< The starting window height.

        i32 swap_interval = 1;         ///< Window swap back-buffer interval.
    };

    struct Window {
        GLFWwindow* handle  = nullptr;
        i32         width   = 0;
        i32         height  = 0;
        bool        resized = false;

        explicit constexpr Window() noexcept = default;

        /// Initialize a window instance.
        void init(WindowConfig const& config) noexcept;

        ~Window() noexcept;

        /// Display the window instance to the screen.
        void show() noexcept;

        /// Check if the window was flagged to be closed.
        [[nodiscard]] bool should_close() noexcept;

        /// Get the state of a given key.
        [[nodiscard]] KeyState key_state(Key k) noexcept;

        /// Update the extent of the window's frame buffer.
        void update_extent() noexcept;

        /// Wait for events if the window is currently minimized.
        void wait_if_minimized() noexcept;

        // TODO: the window shouldn't be responsible for polling the events, only updating itself.
        /// Poll events and update the window.
        void poll_events() noexcept;

        void set_title(strptr title) noexcept;
    };
}  // namespace mina::gfx
