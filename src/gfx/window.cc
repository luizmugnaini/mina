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
/// Description: Implementation of the windowing layer.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/window.h>

#include <mina/meta/info.h>
#include <psh/assert.h>
#include <psh/types.h>

namespace mina::gfx {
    static void glfw_error_callback(i32 error_code, strptr desc) {
        psh::log_fmt(
            psh::LogLevel::Error,
            "[GLFW] error code: %d, description: %s",
            error_code,
            desc);
    }

    void Window::init(WindowConfig const& config) noexcept {
        glfwSetErrorCallback(glfw_error_callback);

        // Initialize GLFW.
        psh_assert_msg(glfwInit() == GLFW_TRUE, "GLFW failed to initialize.");

        psh_assert_msg(glfwVulkanSupported(), "Unable to find a working Vulkan loader");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Set the window invisible so that we can change it's position without it being noticed
        // by the user.
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        // Initialize window.
        //
        // If a width or height (and x, y positions) weren't specified by the callee, we compute
        // them based on the primary monitor information.
        {
            // Get information about the user's primary monitor.
            GLFWmonitor* const primary_monitor = glfwGetPrimaryMonitor();
            i32                monitor_x       = 0;
            i32                monitor_y       = 0;
            i32                monitor_width   = 0;
            i32                monitor_height  = 0;
            glfwGetMonitorWorkarea(
                primary_monitor,
                &monitor_x,
                &monitor_y,
                &monitor_width,
                &monitor_height);

            // Compute the height and width of the window if needed.
            width  = config.width;
            height = config.height;
            if ((width == -1) || (height == -1)) {
                width  = static_cast<i32>(static_cast<f32>(monitor_width) * 0.75f);
                height = static_cast<i32>(static_cast<f32>(width) / 16.0f) * 9;
            }

            // Initialize the window.
            handle = glfwCreateWindow(width, height, EMU_NAME.buf, nullptr, nullptr);
            psh_assert_msg(handle != nullptr, "Unable to create the window.");

            // Compute and set the window position.
            {
                i32 x = config.x;
                i32 y = config.y;
                if ((x == -1) || (y == -1)) {
                    x = monitor_x +
                        static_cast<i32>(static_cast<f32>(monitor_width - width) / 2.0f);
                    y = monitor_y +
                        static_cast<i32>(static_cast<f32>(monitor_height - height) / 2.0f);
                }

                glfwSetWindowPos(handle, x, y);
            }
        }

        psh_assert_msg(
            config.user_pointer != nullptr,
            "A user pointer should be specified to be bound to the GLFW window.");
        glfwSetWindowUserPointer(handle, config.user_pointer);
    }

    Window::~Window() noexcept {
        if (handle != nullptr) {
            glfwDestroyWindow(handle);
        }
        glfwTerminate();
    }

    KeyState Window::key_state(Key k) noexcept {
        return static_cast<KeyState>(glfwGetKey(handle, static_cast<i32>(k)));
    }

    void Window::show() noexcept {
        glfwShowWindow(handle);
    }

    bool Window::should_close() noexcept {
        return glfwWindowShouldClose(handle) == GLFW_TRUE;
    }

    void Window::update_extent() noexcept {
        i32 const prev_width  = width;
        i32 const prev_height = height;
        glfwGetFramebufferSize(handle, &width, &height);
        if ((width != prev_width) || (height != prev_height)) {
            resized = true;
        } else {
            resized = false;
        }
    }

    void Window::wait_if_minimized() noexcept {
        while (width == 0 || height == 0) {
            glfwWaitEvents();
            this->update_extent();
        }
    }

    void Window::poll_events() noexcept {
        glfwPollEvents();
        this->update_extent();
    }

    void Window::set_title(strptr title) noexcept {
        glfwSetWindowTitle(handle, title);
    }
}  // namespace mina::gfx
