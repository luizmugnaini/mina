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
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/window.h>

#include <mina/meta/info.h>
#include <psh/assert.h>
#include <psh/types.h>

namespace mina {
    void glfw_error_callback(i32 error_code, strptr desc) {
        psh_error_fmt("[GLFW] error code: %d, description: %s", error_code, desc);
    }

    void init_window(Window& win, WindowConfig const& config) noexcept {
        glfwSetErrorCallback(glfw_error_callback);

        // Initialize GLFW.
        {
            psh_assert_msg(glfwInit() == GLFW_TRUE, "GLFW failed to initialize.");

            psh_assert_msg(glfwVulkanSupported(), "Unable to find a working Vulkan loader");
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        // Initialize window.
        //
        // If a width or height (and x, y positions) weren't specified by the callee, we compute
        // them based on the primary monitor information.
        {
            // Set the window invisible so that we can change it's position without it being noticed
            // by the user.
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

            // Get information about the user's primary monitor.
            GLFWmonitor* const primary_monitor = glfwGetPrimaryMonitor();
            i32                monitor_x, monitor_y, monitor_width, monitor_height;
            glfwGetMonitorWorkarea(
                primary_monitor,
                &monitor_x,
                &monitor_y,
                &monitor_width,
                &monitor_height);

            win.width =
                config.width.val_or(static_cast<i32>(static_cast<f32>(monitor_width) * 0.75f));
            win.height =
                config.height.val_or(static_cast<i32>(static_cast<f32>(win.width) / 16.0f) * 9);

            // Initialize the window.
            win.handle = glfwCreateWindow(win.width, win.height, EMU_NAME.buf, nullptr, nullptr);
            psh_assert_msg(win.handle != nullptr, "Unable to create the window.");

            // Set the window position.
            win.position.x = config.x.val_or(
                monitor_x + static_cast<i32>(static_cast<f32>(monitor_width - win.width) / 2.0f));
            win.position.y = config.y.val_or(
                monitor_y + static_cast<i32>(static_cast<f32>(monitor_height - win.height) / 2.0f));
            glfwSetWindowPos(win.handle, win.position.x, win.position.y);
        }

        psh_assert_msg(
            config.user_pointer != nullptr,
            "A user pointer should be specified to be bound to the GLFW window.");
        glfwSetWindowUserPointer(win.handle, config.user_pointer);
    }

    void destroy_window(Window& win) noexcept {
        if (win.handle != nullptr) {
            glfwDestroyWindow(win.handle);
        }
        psh::zero_struct(win);  // Not necessary but we do anyways...

        glfwTerminate();
    }

    void set_window_title(Window& win, strptr title) noexcept {
        glfwSetWindowTitle(win.handle, title);
    }

    void update_window_state(Window& win) noexcept {
        // Update window dimensions.
        {
            i32 prev_width  = win.width;
            i32 prev_height = win.height;
            glfwGetFramebufferSize(win.handle, &win.width, &win.height);

            win.resized = ((win.width != prev_width) || (win.height != prev_height));
        }

        // Update position of the window.
        glfwGetWindowPos(win.handle, &win.position.x, &win.position.y);

        // Check if there was any request for closing the window.
        win.should_close = (glfwWindowShouldClose(win.handle) == GLFW_TRUE);
    }

    void display_window(Window& win) noexcept {
        glfwShowWindow(win.handle);
        update_window_state(win);
    }

    void wait_if_minimized(Window& win) noexcept {
        while (win.width == 0 || win.height == 0) {
            glfwWaitEvents();
            update_window_state(win);
        }
    }

    void process_input_events(Window& win) noexcept {
        glfwPollEvents();

        update_window_state(win);
    }
}  // namespace mina
