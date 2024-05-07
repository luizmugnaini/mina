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
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <psh/arena.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

#define mina_vk_assert(res)                                                       \
    do {                                                                          \
        VkResult vkres = res;                                                     \
        psh::assert_(vkres == VK_SUCCESS, #res, "Vulkan operation unsuccessful"); \
    } while (0)

#define mina_vk_assert_msg(res, msg)                  \
    do {                                              \
        VkResult vkres = res;                         \
        psh::assert_(vkres == VK_SUCCESS, #res, msg); \
    } while (0)

#define MINA_CLEAR_COLOR 1.0f, 0.0f, 1.0f, 1.0f

namespace mina::gfx {
    bool has_validation_layers(
        psh::ScratchArena&&       sarena,
        psh::FatPtr<StrPtr const> layers) noexcept;

    bool has_required_extensions(
        psh::ScratchArena&&       sarena,
        psh::FatPtr<StrPtr const> exts) noexcept;

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
}  // namespace mina::gfx
