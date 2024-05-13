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
/// Description: Implementation of the Vulkan graphics utility functions.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/utils.h>

#include <psh/array.h>
#include <psh/io.h>
#include <psh/string.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    bool has_validation_layers(
        psh::ScratchArena&&       sarena,
        psh::FatPtr<strptr const> layers) noexcept {
        u32 avail_layer_count;
        vkEnumerateInstanceLayerProperties(&avail_layer_count, nullptr);

        psh::Array<VkLayerProperties> avail_layers{sarena.arena, avail_layer_count};
        vkEnumerateInstanceLayerProperties(&avail_layer_count, avail_layers.buf);

        for (strptr l : layers) {
            bool found = false;
            for (auto const& al : avail_layers) {
                if (psh::str_equal(l, al.layerName)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                psh::log_fmt(psh::LogLevel::Debug, "Vulkan validation layer '%s' not found", l);
                return false;
            }
        }
        return true;
    }

    bool has_required_extensions(
        psh::ScratchArena&&       sarena,
        psh::FatPtr<strptr const> exts) noexcept {
        u32 avail_ext_count;
        vkEnumerateInstanceExtensionProperties(nullptr, &avail_ext_count, nullptr);

        psh::Array<VkExtensionProperties> avail_ext{sarena.arena, avail_ext_count};
        vkEnumerateInstanceExtensionProperties(nullptr, &avail_ext_count, avail_ext.buf);

        for (strptr e : exts) {
            bool found = false;
            for (auto const& ae : avail_ext) {
                if (psh::str_equal(e, ae.extensionName)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                psh::log_fmt(psh::LogLevel::Debug, "Vulkan extension '%s' not found", e);
                return false;
            }
        }
        return true;
    }

    strptr debug_msg_type_str(VkDebugUtilsMessageTypeFlagsEXT type) noexcept {
        strptr type_str;
        switch (type) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: {
                type_str = "general";
                break;
            }
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: {
                type_str = "validation";
                break;
            }
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: {
                type_str = "performance";
                break;
            }
            case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: {
                type_str = "binding";
                break;
            }
            default: psh_unreachable();
        }
        return type_str;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             type,
        VkDebugUtilsMessengerCallbackDataEXT const* data,
        void* /* unused user_data */) noexcept {
        switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
                // Pass-through.
            }
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
                // Ignore these messages.
                break;
            }
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
                psh::log_fmt(
                    psh::LogLevel::Warning,
                    "[Vulkan][%s] type: %s, message: %s.",
                    "WARNING",
                    debug_msg_type_str(type),
                    data->pMessage);
                break;
            }
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
                psh::log_fmt(
                    psh::LogLevel::Error,
                    "[Vulkan][%s] type: %s, message: %s.",
                    "ERROR",
                    debug_msg_type_str(type),
                    data->pMessage);
                break;
            }
            default: psh_unreachable();
        }
        return VK_FALSE;
    }

    VkResult create_debug_utils_messenger(
        VkInstance                                instance,
        VkDebugUtilsMessengerCreateInfoEXT const& dbg_msg_info,
        VkAllocationCallbacks const*              alloc_cb,
        VkDebugUtilsMessengerEXT&                 dum) noexcept {
        auto* const dum_create_fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        VkResult res;
        if (dum_create_fn == nullptr) {
            psh::log(psh::LogLevel::Error, "Unable to load proc vkCreateDebugUtilsMessengerEXT");
            res = VK_ERROR_EXTENSION_NOT_PRESENT;
        } else {
            res = dum_create_fn(instance, &dbg_msg_info, alloc_cb, &dum);
        }

        return res;
    }

    void destroy_debug_utils_messenger(
        VkInstance                   instance,
        VkDebugUtilsMessengerEXT     dum,
        VkAllocationCallbacks const* alloc_cb) noexcept {
        auto* const dum_destroy_fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (dum_destroy_fn == nullptr) {
            psh::log(
                psh::LogLevel::Error,
                "Unable to load the proc vkDestroyDebugUtilsMessengerEXT.");
        } else {
            dum_destroy_fn(instance, dum, alloc_cb);
        }
    }
}  // namespace mina::gfx
