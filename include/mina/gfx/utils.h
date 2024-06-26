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
/// Description: Vulkan graphics utility functions and macros.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <psh/arena.h>
#include <psh/log.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

#if defined(MINA_DEBUG) || defined(MINA_VULKAN_DEBUG)
#    define mina_vk_assert(res)                       \
        do {                                          \
            VkResult vkres = (res);                   \
            if (vkres != VK_SUCCESS) {                \
                psh::log_fmt(                         \
                    {psh::LogLevel::LEVEL_FATAL},     \
                    psh::ASSERT_FMT,                  \
                    #res,                             \
                    "Vulkan operation unsuccessful"); \
                psh_abort();                          \
            }                                         \
        } while (0)
#    define mina_vk_assert_msg(res, msg)                                                \
        do {                                                                            \
            VkResult vkres = (res);                                                     \
            if (vkres != VK_SUCCESS) {                                                  \
                psh::log_fmt(psh::LogLevel::LEVEL_FATAL, psh::ASSERT_FMT, #res, (msg)); \
                psh_abort();                                                            \
            }                                                                           \
        } while (0)
#else
#    define mina_vk_assert(res)          (void)(res)
#    define mina_vk_assert_msg(res, msg) (void)(res)
#endif

#define MINA_CLEAR_COLOR 1.0f, 0.0f, 1.0f, 1.0f

namespace mina {
    bool has_validation_layers(
        psh::ScratchArena&&       sarena,
        psh::FatPtr<strptr const> layers) noexcept;

    bool has_required_extensions(
        psh::ScratchArena&&       sarena,
        psh::FatPtr<strptr const> exts) noexcept;

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             type,
        VkDebugUtilsMessengerCallbackDataEXT const* data,
        void* /* unused user_data */) noexcept;

    VkResult create_debug_utils_messenger(
        VkInstance                                instance,
        VkDebugUtilsMessengerCreateInfoEXT const& dbg_msg_info,
        VkAllocationCallbacks const*              alloc_cb,
        VkDebugUtilsMessengerEXT&                 dum) noexcept;

    void destroy_debug_utils_messenger(
        VkInstance                   instance,
        VkDebugUtilsMessengerEXT     dum,
        VkAllocationCallbacks const* alloc_cb) noexcept;
}  // namespace mina
