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
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/gfx/context.h>

#include <mina/gfx/buffer.h>
#include <mina/gfx/command.h>
#include <mina/gfx/pipeline.h>
#include <mina/gfx/swap_chain.h>
#include <mina/gfx/sync.h>
#include <mina/gfx/utils.h>
#include <mina/gfx/vma.h>
#include <mina/meta/info.h>
#include <psh/algorithms.h>
#include <psh/arena.h>
#include <psh/dyn_array.h>
#include <psh/intrinsics.h>
#include <psh/option.h>
#include <psh/string.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

namespace mina {
    // -----------------------------------------------------------------------------
    // - Internal implementation details -
    // -----------------------------------------------------------------------------

    namespace {
        // Stores indices to specific family queues.
        //
        // These indices may coincide, so in order to get them without repetition you'll need to
        // call `QueueFamilies::unique_indices`.
        struct QueueFamiliesQuery {
            psh::Option<u32> graphics_idx = {};
            psh::Option<u32> present_idx  = {};

            void
            query(psh::ScratchArena&& sarena, VkPhysicalDevice pdev, VkSurfaceKHR surf) noexcept {
                u32 fam_count;
                vkGetPhysicalDeviceQueueFamilyProperties(pdev, &fam_count, nullptr);

                psh::Array<VkQueueFamilyProperties> fam_props{sarena.arena, fam_count};
                vkGetPhysicalDeviceQueueFamilyProperties(pdev, &fam_count, fam_props.buf);

                for (u32 i = 0; i < fam_props.size; ++i) {
                    if ((fam_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                        graphics_idx = i;
                    }

                    u32 present_support;
                    mina_vk_assert(
                        vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, surf, &present_support));
                    if (present_support == VK_TRUE) {
                        present_idx = i;
                    }

                    if (this->has_all()) {
                        break;
                    }
                }
            }

            bool has_all() const noexcept {
                return graphics_idx.has_val && present_idx.has_val;
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
                    psh_debug_fmt("Vulkan physical device extension '%s' not found.", ext);
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
            QueueFamiliesQuery qfq   = {};
            for (VkPhysicalDevice pdev : pdevs) {
                // Get Information.
                VkPhysicalDeviceFeatures feats;
                vkGetPhysicalDeviceFeatures(pdev, &feats);
                qfq.query(arena->make_scratch(), pdev, ctx.surf);
                query_swap_chain_info(pdev, ctx.surf, arena, swc_info);

                // Check requirements.
                bool supports_geom = (feats.geometryShader == VK_TRUE);
                bool has_exts = physical_device_has_extensions(arena->make_scratch(), pdev, exts);
                bool has_all_queue_fams = qfq.has_all();
                bool minimal_swc        = !(
                    swc_info.surface_formats.is_empty() || swc_info.presentation_modes.is_empty());

                // NOTE: It is likely that the first device will be the one selected.
                if (psh_unlikely(
                        !(supports_geom && has_exts && has_all_queue_fams && minimal_swc))) {
                    continue;
                }

                // Fill the info for the selected device.
                ctx.pdev                        = pdev;
                ctx.queues.graphics_queue_index = qfq.graphics_idx.val;
                ctx.queues.present_queue_index  = qfq.present_idx.val;

                found = true;
                break;
            }

            return found;
        }

        VkDevice create_logical_device(
            VkPhysicalDevice          pdev,
            QueueFamilies&            queues,
            psh::ScratchArena&&       sarena,
            psh::FatPtr<strptr const> exts) {
            psh::DynArray<u32> uidx{sarena.arena, 2};
            uidx.push(queues.graphics_queue_index);
            if (psh_unlikely(queues.present_queue_index != queues.graphics_queue_index)) {
                uidx.push(queues.present_queue_index);
            }

            psh::Array<f32> priorities{sarena.arena, uidx.size};
            psh::fill(psh::fat_ptr(priorities), 1.0f);

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

            VkDevice           dev;
            VkDeviceCreateInfo dev_info{
                .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount    = static_cast<u32>(dev_queue_info.size),
                .pQueueCreateInfos       = dev_queue_info.buf,
                .enabledExtensionCount   = static_cast<u32>(exts.size),
                .ppEnabledExtensionNames = exts.buf,
            };
            mina_vk_assert(vkCreateDevice(pdev, &dev_info, nullptr, &dev));

            return dev;
        }
    }  // namespace

    // -----------------------------------------------------------------------------
    // - Graphics context lifetime management implementation -
    // -----------------------------------------------------------------------------

    void init_graphics_system(
        GraphicsContext& ctx,
        WindowHandle*    win_handle,
        psh::Arena*      persistent_arena,
        psh::Arena*      work_arena) noexcept {
        psh_assert_msg(
            persistent_arena != nullptr,
            "Persistent arena required for the graphics context");
        psh_assert_msg(work_arena != nullptr, "Work arena required for the graphics context");
        ctx.persistent_arena = persistent_arena;
        ctx.work_arena       = work_arena;

        // Initialize the vulkan instance.
        {
            psh::ScratchArena sarena = ctx.work_arena->make_scratch();

            // Get GLFW required extensions.
            u32     glfw_ext_count = 0;
            strptr* glfw_ext       = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

            psh::DynArray<strptr> required_extensions{
                psh::FatPtr<strptr const>{glfw_ext, glfw_ext_count},
                sarena.arena,
                glfw_ext_count + 1};
            constexpr strptr validation_layer = "VK_LAYER_KHRONOS_validation";

#if defined(MINA_DEBUG) || defined(MINA_VULKAN_DEBUG)
            required_extensions.push("VK_EXT_debug_utils");
            psh_assert_msg(
                has_validation_layers(sarena.decouple(), psh::FatPtr{&validation_layer, 1}),
                "Vulkan validation layer support is inexistent in the current device");
#endif  // MINA_VULKAN_DEBUG

            psh_assert_msg(
                has_required_extensions(sarena.decouple(), const_fat_ptr(required_extensions)),
                "Unable to find all required Vulkan extensions");

            VkApplicationInfo app_info{
                .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName   = EMU_NAME.buf,
                .applicationVersion = VK_MAKE_VERSION(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION),
                .pEngineName        = ENGINE_NAME.buf,
                .apiVersion         = MINA_VULKAN_API_VERSION,
            };
            VkInstanceCreateInfo instance_info{
                .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pApplicationInfo        = &app_info,
                .enabledLayerCount       = 1,
                .ppEnabledLayerNames     = &validation_layer,
                .enabledExtensionCount   = static_cast<u32>(required_extensions.size),
                .ppEnabledExtensionNames = required_extensions.buf,
            };

#if defined(MINA_DEBUG) || defined(MINA_VULKAN_DEBUG)
            VkDebugUtilsMessengerCreateInfoEXT dbg_msg_create_info{
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
#endif

            mina_vk_assert(vkCreateInstance(&instance_info, nullptr, &ctx.instance));

#if defined(MINA_DEBUG) || defined(MINA_VULKAN_DEBUG)
            mina_vk_assert(create_debug_utils_messenger(
                ctx.instance,
                dbg_msg_create_info,
                nullptr,
                ctx.dbg_msg));
#endif
        }

        // Surface creation.
        mina_vk_assert(glfwCreateWindowSurface(ctx.instance, win_handle, nullptr, &ctx.surf));

        SwapChainInfo     swc_info = {};
        psh::ScratchArena sarena   = ctx.work_arena->make_scratch();

        // Select the physical device having the requested extensions, and get its swap chain info.
        {
            constexpr psh::Buffer<strptr, 2> pdev_ext{"VK_KHR_swapchain", "VK_EXT_memory_budget"};

            psh_assert_msg(
                select_physical_dev(ctx, sarena.arena, const_fat_ptr(pdev_ext), swc_info),
                "Failed to find an adequate physical device for the Vulkan graphics context.");

            ctx.dev = create_logical_device(
                ctx.pdev,
                ctx.queues,
                sarena.decouple(),
                const_fat_ptr(pdev_ext));
        }

        // Setup queues.
        vkGetDeviceQueue(ctx.dev, ctx.queues.graphics_queue_index, 0, &ctx.queues.graphics_queue);
        vkGetDeviceQueue(ctx.dev, ctx.queues.present_queue_index, 0, &ctx.queues.present_queue);

        // Create the allocator for the graphics context.
        {
            VmaVulkanFunctions vk_fns{
                .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr   = &vkGetDeviceProcAddr,
            };
            VmaDeviceMemoryCallbacks host_mem_cb =
                {};  // TODO: fill with custom host memory callbacks.
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
            vmaCreateAllocator(&alloc_info, &ctx.alloc);
        }

        create_swap_chain(ctx.dev, ctx.surf, ctx.swap_chain, ctx.queues, win_handle, swc_info);

        create_image_views(ctx.dev, ctx.swap_chain, ctx.persistent_arena);

        // Create vertex and uniform buffers.
        create_buffers(ctx.alloc, ctx.buffers, ctx.queues);

        // Create the descriptor set for the uniform buffer.
        create_descriptor_sets(ctx.dev, ctx.descriptor_sets, ctx.buffers.uniform_buffer());

        create_graphics_pipeline_context(
            ctx.dev,
            ctx.persistent_arena,
            ctx.pipelines.graphics,
            ctx.descriptor_sets,
            ctx.swap_chain.surface_format.format,
            ctx.swap_chain.extent);

        create_frame_buffers(
            ctx.dev,
            ctx.swap_chain,
            ctx.persistent_arena,
            ctx.pipelines.graphics.render_pass);

        create_command_buffers(
            ctx.dev,
            ctx.commands,
            ctx.queues,
            ctx.persistent_arena,
            ctx.swap_chain.max_frames_in_flight);

        create_synchronizers(
            ctx.dev,
            ctx.sync,
            ctx.persistent_arena,
            ctx.swap_chain.max_frames_in_flight);
    }

    void destroy_graphics_system(GraphicsContext& ctx) noexcept {
        vkDeviceWaitIdle(ctx.dev);  // Wait for the device to finish all operations.
        destroy_synchronizers(ctx.dev, ctx.sync);

        destroy_command_buffers(ctx.dev, ctx.commands);
        destroy_descriptor_sets(ctx.dev, ctx.descriptor_sets);
        destroy_graphics_pipeline_context(ctx.dev, ctx.pipelines.graphics);
        destroy_swap_chain(ctx.dev, ctx.swap_chain);

        destroy_buffers(ctx.alloc, ctx.buffers);
        vmaDestroyAllocator(ctx.alloc);

        vkDestroyDevice(ctx.dev, nullptr);

#if defined(MINA_DEBUG) || defined(MINA_VULKAN_DEBUG)
        destroy_debug_utils_messenger(ctx.instance, ctx.dbg_msg, nullptr);
#endif

        vkDestroySurfaceKHR(ctx.instance, ctx.surf, nullptr);
        vkDestroyInstance(ctx.instance, nullptr);
    }

    void recreate_swap_chain_context(GraphicsContext& ctx, Window& win) noexcept {
        wait_if_minimized(win);

        // Complete all operations before proceeding
        vkDeviceWaitIdle(ctx.dev);

        // Recreate the swap chain itself.
        {
            destroy_swap_chain(ctx.dev, ctx.swap_chain);

            psh::ScratchArena sarena = ctx.work_arena->make_scratch();
            SwapChainInfo     swc_info;
            query_swap_chain_info(ctx.pdev, ctx.surf, sarena.arena, swc_info);

            create_swap_chain(ctx.dev, ctx.surf, ctx.swap_chain, ctx.queues, win.handle, swc_info);
        }

        // Recreate the swap chain's associated context objects.
        recreate_image_views(ctx.dev, ctx.swap_chain);
        recreate_frame_buffers(ctx.dev, ctx.swap_chain, ctx.pipelines.graphics.render_pass);
    }

    FrameResources current_frame_resources(GraphicsContext const& ctx) noexcept {
        u32 current_frame = ctx.swap_chain.current_frame;
        return FrameResources{
            .transfer_cmd              = ctx.commands.transfer.cmd[current_frame],
            .graphics_cmd              = ctx.commands.graphics.cmd[current_frame],
            .frame_in_flight_fence     = ctx.sync.frame_in_flight.frame_fence[current_frame],
            .transfer_ended_fence      = ctx.sync.finished_transfer.frame_fence[current_frame],
            .image_available_semaphore = ctx.sync.image_available.frame_semaphore[current_frame],
            .render_pass_ended_semaphore =
                ctx.sync.finished_render_pass.frame_semaphore[current_frame],
        };
    }
}  // namespace mina
