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
/// Description: Implementation of the Vulkan graphics application context.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/context.h>

#include <mina/gfx/pipeline.h>
#include <mina/gfx/swap_chain.h>
#include <mina/gfx/sync.h>
#include <mina/gfx/utils.h>
#include <mina/gfx/vma.h>
#include <mina/meta/info.h>
#include <psh/arena.h>
#include <psh/dyn_array.h>
#include <psh/intrinsics.h>
#include <psh/option.h>
#include <psh/string.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    namespace {
        // Stores indices to specific family queues.
        //
        // These indices may coincide, so in order to get them without repetition you'll need to
        // call `QueueFamilies::unique_indices`.
        struct QueueFamiliesQuery {
            psh::Option<u32> graphics_idx{};
            psh::Option<u32> transfer_idx{};
            psh::Option<u32> present_idx{};

            void
            query(psh::ScratchArena&& sarena, VkPhysicalDevice pdev, VkSurfaceKHR surf) noexcept {
                u32 fam_count;
                vkGetPhysicalDeviceQueueFamilyProperties(pdev, &fam_count, nullptr);

                psh::Array<VkQueueFamilyProperties> fam_props{sarena.arena, fam_count};
                vkGetPhysicalDeviceQueueFamilyProperties(pdev, &fam_count, fam_props.buf);

                for (u32 i = 0; i < fam_props.size; ++i) {
                    // NOTE: Prefer separate queues for graphics and transfer.
                    if ((fam_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                        graphics_idx = i;
                    } else if ((fam_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) {
                        transfer_idx = i;
                    }

                    u32 present_support;
                    mina_vk_assert(
                        vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, surf, &present_support));
                    if (present_support == VK_TRUE) {
                        present_idx = i;
                    }

                    if (this->has_all()) break;
                }

                // If we couldn't find a separate queue the transfer, use the graphics one.
                if (!transfer_idx.has_val) {
                    transfer_idx = graphics_idx;
                }
            }

            bool has_all() const noexcept {
                return graphics_idx.has_val && transfer_idx.has_val && present_idx.has_val;
            }
        };

        bool physical_device_has_extensions(
            psh::ScratchArena&&       sarena,
            VkPhysicalDevice          pdev,
            psh::FatPtr<strptr const> exts) noexcept {
            u32 avail_ext_count = 0;
            vkEnumerateDeviceExtensionProperties(pdev, nullptr, &avail_ext_count, nullptr);

            psh::Array<VkExtensionProperties> avail_exts{sarena.arena, avail_ext_count};
            vkEnumerateDeviceExtensionProperties(pdev, nullptr, &avail_ext_count, avail_exts.buf);

            for (strptr ext : exts) {
                bool found = false;
                for (auto const& aexts : avail_exts) {
                    if (psh::str_equal(ext, aexts.extensionName)) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    log_fmt(
                        psh::LogLevel::Debug,
                        "Vulkan physical device extension '%s' not found.",
                        ext);
                    return false;
                }
            }
            return true;
        }

        // Selects a physical device according a set of requirements:
        // - Support for geometry shaders.
        // - Support for the required extensions.
        // - Has at least a minimal support for graphics and presentation.
        bool select_physical_dev(
            GraphicsContext&          ctx,
            psh::Arena*               arena,
            psh::FatPtr<strptr const> exts,
            SwapChainInfo&            swc_info) noexcept {
            u32 pdev_count;
            psh_discard(vkEnumeratePhysicalDevices(ctx.instance, &pdev_count, nullptr));

            psh::Array<VkPhysicalDevice> pdevs{arena, pdev_count};
            mina_vk_assert(vkEnumeratePhysicalDevices(ctx.instance, &pdev_count, pdevs.buf));

            bool               found = false;
            QueueFamiliesQuery qfq{};
            for (VkPhysicalDevice pdev : pdevs) {
                // Get Information.
                VkPhysicalDeviceFeatures feats;
                vkGetPhysicalDeviceFeatures(pdev, &feats);
                qfq.query(arena->make_scratch(), pdev, ctx.surf);
                query_swap_chain_info(arena, pdev, ctx.surf, swc_info);

                // Check requirements.
                bool const supports_geom = (feats.geometryShader == VK_TRUE);
                bool const has_exts =
                    physical_device_has_extensions(arena->make_scratch(), pdev, exts);
                bool const has_all_queue_fams = qfq.has_all();
                bool const minimal_swc =
                    !(swc_info.surf_fmt.is_empty() || swc_info.pres_modes.is_empty());

                // NOTE: It is likely that the first device will be the one selected.
                if (psh_unlikely(
                        !(supports_geom && has_exts && has_all_queue_fams && minimal_swc))) {
                    continue;
                }

                // Fill the info for the selected device.
                ctx.pdev                = pdev;
                ctx.queues.graphics_idx = qfq.graphics_idx.val;
                ctx.queues.transfer_idx = qfq.transfer_idx.val;
                ctx.queues.present_idx  = qfq.present_idx.val;

                found = true;
                break;
            }

            return found;
        }

        void create_logical_device(GraphicsContext& ctx, psh::FatPtr<strptr const> exts) {
            auto sarena = ctx.work_arena->make_scratch();

            psh::DynArray<u32> uidx{sarena.arena, 3};
            uidx.push(ctx.queues.graphics_idx);
            if (ctx.queues.transfer_idx != ctx.queues.graphics_idx) {
                uidx.push(ctx.queues.transfer_idx);
            }
            if (psh_unlikely(ctx.queues.present_idx != ctx.queues.graphics_idx)) {
                if (ctx.queues.present_idx != ctx.queues.transfer_idx) {
                    uidx.push(ctx.queues.present_idx);
                }
            }

            psh::Array<f32> priorities{sarena.arena, uidx.size};
            priorities.fill(1.0f);

            // Construct all device queue family informations.
            psh::Array<VkDeviceQueueCreateInfo> dev_queue_info{sarena.arena, uidx.size};
            for (u32 i = 0; i < uidx.size; ++i) {
                dev_queue_info[i] = VkDeviceQueueCreateInfo{
                    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = uidx[i],
                    .queueCount       = 1,
                    .pQueuePriorities = priorities.buf,
                };
            }

            VkDeviceCreateInfo const dev_info{
                .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount    = static_cast<u32>(dev_queue_info.size),
                .pQueueCreateInfos       = dev_queue_info.buf,
                .enabledExtensionCount   = static_cast<u32>(exts.size),
                .ppEnabledExtensionNames = exts.buf,
            };

            mina_vk_assert(vkCreateDevice(ctx.pdev, &dev_info, nullptr, &ctx.dev));
        }

        void initialize_vulkan(GraphicsContext& ctx) noexcept {
            psh::ScratchArena sarena = ctx.work_arena->make_scratch();

            // Get GLFW required extensions.
            u32                   glfw_ext_count = 0;
            strptr* const         glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
            psh::DynArray<strptr> required_extensions{
                psh::FatPtr<strptr const>{glfw_ext, glfw_ext_count},
                sarena.arena,
                glfw_ext_count + 1};

            constexpr strptr validation_layer = "VK_LAYER_KHRONOS_validation";

#if defined(MINA_VULKAN_DEBUG)
            required_extensions.push("VK_EXT_debug_utils");
            psh_assert_msg(
                has_validation_layers(sarena.decouple(), psh::FatPtr{&validation_layer, 1}),
                "Vulkan validation layer support is inexistent in the current device");
#endif  // MINA_VULKAN_DEBUG

            psh_assert_msg(
                has_required_extensions(sarena.decouple(), required_extensions.const_fat_ptr()),
                "Unable to find all required Vulkan extensions");

            VkApplicationInfo const app_info{
                .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName   = EMU_NAME.buf,
                .applicationVersion = VK_MAKE_VERSION(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION),
                .pEngineName        = ENGINE_NAME.buf,
                .apiVersion         = VK_API_VERSION_1_3,
            };
            VkInstanceCreateInfo instance_info{
                .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pApplicationInfo        = &app_info,
                .enabledLayerCount       = 1,
                .ppEnabledLayerNames     = &validation_layer,
                .enabledExtensionCount   = static_cast<u32>(required_extensions.size),
                .ppEnabledExtensionNames = required_extensions.buf,
            };

#if defined(MINA_VULKAN_DEBUG)
            VkDebugUtilsMessengerCreateInfoEXT const dbg_msg_create_info{
                .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
                .pfnUserCallback = debug_callback,
                .pUserData       = &ctx,
            };

            instance_info.pNext = &dbg_msg_create_info;
#endif  // MINA_VULKAN_DEBUG

            mina_vk_assert(vkCreateInstance(&instance_info, nullptr, &ctx.instance));

#if defined(MINA_VULKAN_DEBUG)
            mina_vk_assert(create_debug_utils_messenger(
                ctx.instance,
                dbg_msg_create_info,
                nullptr,
                ctx.dbg_msg));
#endif  // MINA_VULKAN_DEBUG
        }
    }  // namespace

    void init_graphics_context(GraphicsContext& ctx, GraphicsContextConfig const& config) noexcept {
        ctx.persistent_arena = config.persistent_arena;
        ctx.work_arena       = config.work_arena;

        ctx.window.init(config.win_config);

        // Initialize a Vulkan instance.
        initialize_vulkan(ctx);

        // Surface creation.
        mina_vk_assert(
            glfwCreateWindowSurface(ctx.instance, ctx.window.handle, nullptr, &ctx.surf));

        auto sarena = ctx.work_arena->make_scratch();

        SwapChainInfo                    swc_info{};
        constexpr psh::Buffer<strptr, 2> pdev_ext{"VK_KHR_swapchain", "VK_EXT_memory_budget"};
        psh_assert_msg(
            select_physical_dev(ctx, sarena.arena, pdev_ext.const_fat_ptr(), swc_info),
            "Failed to find an adequate physical device for the Vulkan graphics context.");

        create_logical_device(ctx, pdev_ext.const_fat_ptr());

        // Setup queues.
        {
            vkGetDeviceQueue(ctx.dev, ctx.queues.graphics_idx, 0, &ctx.queues.graphics);
            vkGetDeviceQueue(ctx.dev, ctx.queues.transfer_idx, 0, &ctx.queues.transfer);
            vkGetDeviceQueue(ctx.dev, ctx.queues.present_idx, 0, &ctx.queues.present);
        }

        // Create the allocator for the graphics context.
        {
            VmaVulkanFunctions vk_fns{
                .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr   = &vkGetDeviceProcAddr,
            };
            VmaDeviceMemoryCallbacks
                                   host_mem_cb{};  // TODO: fill with custom host memory callbacks.
            VmaAllocatorCreateInfo alloc_info{
                .flags                       = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
                .physicalDevice              = ctx.pdev,
                .device                      = ctx.dev,
                .preferredLargeHeapBlockSize = 0,  // Defaults to 256 MiB.
                .pDeviceMemoryCallbacks      = &host_mem_cb,
                .pVulkanFunctions            = &vk_fns,
                .instance                    = ctx.instance,
                .vulkanApiVersion            = MINA_VULKAN_API_VERSION,
            };
            vmaCreateAllocator(&alloc_info, &ctx.gpu_allocator);
        }

        create_swap_chain(ctx, swc_info);
        create_image_views(ctx);
        create_graphics_pipeline(ctx);
        create_frame_buffers(ctx);
        create_graphics_commands(ctx);

        create_synchronizers(ctx);
    }

    void destroy_graphics_context(GraphicsContext& ctx) noexcept {
        // Wait for the device to finish all operations.
        vkDeviceWaitIdle(ctx.dev);

        destroy_synchronizers(ctx);

        vkDestroyCommandPool(ctx.dev, ctx.cmd.pool, nullptr);

        destroy_graphics_pipeline(ctx);

        vmaDestroyBuffer(
            ctx.gpu_allocator,
            ctx.buffers.staging.handle,
            ctx.buffers.staging.allocation);
        vmaDestroyBuffer(ctx.gpu_allocator, ctx.buffers.data.handle, ctx.buffers.data.allocation);

        destroy_swap_chain(ctx);

        vkDestroyDevice(ctx.dev, nullptr);

        vmaDestroyAllocator(ctx.gpu_allocator);

#if defined(MINA_VULKAN_DEBUG)
        destroy_debug_utils_messenger(ctx.instance, ctx.dbg_msg, nullptr);
#endif  // MINA_VULKAN_DEBUG

        vkDestroySurfaceKHR(ctx.instance, ctx.surf, nullptr);
        vkDestroyInstance(ctx.instance, nullptr);
    }
}  // namespace mina::gfx
