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
#include <psh/option.h>
#include <psh/string.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    /// Stores indices to specific family queues.
    ///
    /// These indices may coincide, so in order to get them without repetition you'll need to
    /// call `QueueFamilies::unique_indices`.
    struct QueueFamiliesInfo {
        psh::Option<u32> graphics_idx{};
        psh::Option<u32> transfer_idx{};
        psh::Option<u32> present_idx{};

        void query(psh::ScratchArena&& sarena, VkPhysicalDevice pdev, VkSurfaceKHR surf) noexcept {
            u32 fam_count;
            vkGetPhysicalDeviceQueueFamilyProperties(pdev, &fam_count, nullptr);

            psh::Array<VkQueueFamilyProperties> fam_props{sarena.arena, fam_count};
            vkGetPhysicalDeviceQueueFamilyProperties(pdev, &fam_count, fam_props.buf);

            for (u32 i = 0; i < fam_props.size; ++i) {
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

                if (this->has_all()) {
                    break;
                }
            }

            // If we couldn't find a separate queue the transfer, use the graphics one.
            if (!transfer_idx.has_val) {
                transfer_idx = graphics_idx;
            }
        }

        bool has_all() const noexcept {
            return graphics_idx.has_val && transfer_idx.has_val && present_idx.has_val;
        }

        psh::DynArray<u32> unique_indices(psh::Arena* arena) const noexcept {
            psh::DynArray<u32> uidx{arena, 2};
            if (graphics_idx.has_val) {
                uidx.push(graphics_idx.val);
            }

            if (present_idx.has_val && !contains(present_idx.val, uidx.const_fat_ptr())) {
                uidx.push(present_idx.val);
            }

            return uidx;
        }
    };

    struct PhysicalDeviceInfo {
        VkPhysicalDevice  pdev;
        SwapChainInfo     swc_info;
        QueueFamiliesInfo queue_fam_info;
    };

    static bool physical_device_has_extensions(
        psh::ScratchArena&&       sarena,
        VkPhysicalDevice          pdev,
        psh::FatPtr<StrPtr const> exts) noexcept {
        u32 avail_ext_count = 0;
        vkEnumerateDeviceExtensionProperties(pdev, nullptr, &avail_ext_count, nullptr);

        psh::Array<VkExtensionProperties> avail_exts{sarena.arena, avail_ext_count};
        vkEnumerateDeviceExtensionProperties(pdev, nullptr, &avail_ext_count, avail_exts.buf);

        for (StrPtr e : exts) {
            bool found = false;
            for (auto const& ae : avail_exts) {
                if (psh::str_equal(e, ae.extensionName)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                log_fmt(
                    psh::LogLevel::Debug,
                    "Vulkan physical device extension '%s' not found.",
                    e);
                return false;
            }
        }
        return true;
    }

    static bool query_physical_dev_info(
        psh::Arena*               arena,
        VkInstance                instance,
        VkSurfaceKHR              surf,
        psh::FatPtr<StrPtr const> exts,
        PhysicalDeviceInfo&       info) noexcept {
        u32 pdev_count;
        psh_discard(vkEnumeratePhysicalDevices(instance, &pdev_count, nullptr));

        psh::Array<VkPhysicalDevice> pdevs{arena, pdev_count};
        mina_vk_assert(vkEnumeratePhysicalDevices(instance, &pdev_count, pdevs.buf));

        bool found = false;
        for (VkPhysicalDevice pdev : pdevs) {
            // Geometry shaders.
            VkPhysicalDeviceFeatures feats;
            vkGetPhysicalDeviceFeatures(pdev, &feats);
            bool const supports_geom = feats.geometryShader == VK_TRUE;

            // Required extensions.
            bool const has_exts = physical_device_has_extensions(arena->make_scratch(), pdev, exts);

            // Required queue families.
            info.queue_fam_info.query(arena->make_scratch(), pdev, surf);
            bool const has_all_queue_fams = info.queue_fam_info.has_all();

            // Required swap chain with minimal support for graphics and presentation.
            query_swap_chain_info(arena, pdev, surf, info.swc_info);
            bool const minimal_swc =
                !(info.swc_info.surf_fmt.is_empty() || info.swc_info.pres_modes.is_empty());

            // NOTE: We postpone any early 'continues' to minimize branching. Moreover, the first
            //       physical device will normally be the one we choose anyway.
            if (!(supports_geom && has_exts && has_all_queue_fams && minimal_swc)) {
                continue;
            }

            info.pdev = pdev;
            found     = true;
            break;
        }

        return found;
    }

    static void create_logical_device(
        QueueFamiliesInfo const&  qf_info,
        psh::FatPtr<StrPtr const> exts,
        GraphicsContext&          ctx) {
        auto sarena = ctx.work_arena->make_scratch();

        psh::DynArray<u32> const unique_indices       = qf_info.unique_indices(sarena.arena);
        usize const              unique_indices_count = unique_indices.size;

        psh::Array<f32> priorities{sarena.arena, unique_indices_count};
        priorities.fill(1.0f);

        // Construct all device queue family informations.
        psh::Array<VkDeviceQueueCreateInfo> dev_queue_info{sarena.arena, unique_indices_count};
        for (u32 i = 0; i < unique_indices_count; ++i) {
            dev_queue_info[i] = VkDeviceQueueCreateInfo{
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = unique_indices[i],
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

        // Setup queues.
        ctx.queues.graphics_idx = qf_info.graphics_idx.demand();
        ctx.queues.transfer_idx = qf_info.transfer_idx.demand();
        ctx.queues.present_idx  = qf_info.present_idx.demand();
        vkGetDeviceQueue(ctx.dev, ctx.queues.graphics_idx, 0, &ctx.queues.graphics);
        vkGetDeviceQueue(ctx.dev, ctx.queues.transfer_idx, 0, &ctx.queues.transfer);
        vkGetDeviceQueue(ctx.dev, ctx.queues.present_idx, 0, &ctx.queues.present);
    }

    static void initialize_vulkan(psh::ScratchArena&& sarena, GraphicsContext& ctx) noexcept {
        u32                   glfw_ext_count = 0;
        StrPtr* const         glfw_ext       = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
        psh::DynArray<StrPtr> required_extensions{
            psh::FatPtr{glfw_ext, glfw_ext_count},
            sarena.arena,
            glfw_ext_count + 1};
        constexpr StrPtr validation_layer = "VK_LAYER_KHRONOS_validation";

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
            .pApplicationName   = EMU_NAME,
            .applicationVersion = VK_MAKE_VERSION(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION),
            .pEngineName        = ENGINE_NAME,
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
        mina_vk_assert(
            create_debug_utils_messenger(ctx.instance, dbg_msg_create_info, nullptr, ctx.dbg_msg));
#endif  // MINA_VULKAN_DEBUG
    }

    void init_graphics_context(GraphicsContextConfig const& config, GraphicsContext& ctx) noexcept {
        ctx.persistent_arena = config.persistent_arena;
        ctx.work_arena       = config.work_arena;

        ctx.window.init(config.win_config);

        // Initialize a Vulkan instance.
        initialize_vulkan(ctx.work_arena->make_scratch(), ctx);

        // Surface creation.
        mina_vk_assert(
            glfwCreateWindowSurface(ctx.instance, ctx.window.handle, nullptr, &ctx.surf));

        auto sarena = ctx.work_arena->make_scratch();

        // Query physical device information.
        constexpr psh::Buffer<StrPtr, 2> pdev_ext{"VK_KHR_swapchain", "VK_EXT_memory_budget"};
        PhysicalDeviceInfo               pdev_info;
        psh_assert(query_physical_dev_info(
            sarena.arena,
            ctx.instance,
            ctx.surf,
            pdev_ext.const_fat_ptr(),
            pdev_info));

        ctx.pdev = pdev_info.pdev;
        create_logical_device(pdev_info.queue_fam_info, pdev_ext.const_fat_ptr(), ctx);

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

        create_swap_chain(pdev_info.swc_info, ctx);
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
