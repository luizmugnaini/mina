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
/// Description: Implementation of the Vulkan graphics swap chain management layer.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/gfx/swap_chain.h>

#include <mina/gfx/utils.h>
#include <psh/buffer.h>
#include <psh/intrinsics.h>
#include <psh/mem_utils.h>
#include <cstring>

namespace mina {
    // Does the same as `create_image_views` but without initializing the image and image view
    // arrays, simply reusing and overriding their memory.
    void recreate_image_views(VkDevice dev, SwapChain& swc) noexcept {
        u32 img_count = static_cast<u32>(swc.images.size);

        vkGetSwapchainImagesKHR(dev, swc.handle, &img_count, swc.images.buf);

        VkImageViewCreateInfo img_info{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = swc.surface_format.format,
            .components       = IMAGE_COMPONENT_MAPPING,
            .subresourceRange = IMAGE_SUBRESOURCE_RANGE,
        };

        for (u32 idx = 0; idx < img_count; ++idx) {
            img_info.image = swc.images[idx];
            mina_vk_assert(vkCreateImageView(dev, &img_info, nullptr, &swc.image_views[idx]));
        }
    }

    // NOTE: Does the same as `create_frame_buffers` but without initializing the frame buffer
    // array,
    //       simply reusing and overriding its memory.
    void recreate_frame_buffers(VkDevice dev, SwapChain& swc, RenderPass const& gfx_pass) noexcept {
        VkFramebufferCreateInfo fb_info{
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = gfx_pass.handle,
            .attachmentCount = 1,
            .pAttachments    = nullptr,  // NOTE: Filled for each frame buffer in the loop below.
            .width           = swc.extent.width,
            .height          = swc.extent.height,
            .layers          = 1,
        };

        // Create new frame buffers by overwriting the old ones.
        for (u32 idx = 0; idx < swc.image_views.size; ++idx) {
            fb_info.pAttachments = &swc.image_views[idx];
            mina_vk_assert(vkCreateFramebuffer(dev, &fb_info, nullptr, &swc.frame_bufs[idx]));
        }
    }

    void query_swap_chain_info(
        VkPhysicalDevice pdev,
        VkSurfaceKHR     surf,
        psh::Arena*      arena,
        SwapChainInfo&   swc_info) noexcept {
        // Clear all previous queries.
        std::memset(reinterpret_cast<void*>(&swc_info), 0, sizeof(SwapChainInfo));

        // Surface formats.
        {
            u32 fmt_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &fmt_count, nullptr);

            swc_info.surface_formats.init(arena, fmt_count);
            mina_vk_assert(vkGetPhysicalDeviceSurfaceFormatsKHR(
                pdev,
                surf,
                &fmt_count,
                swc_info.surface_formats.buf));
        }

        // Presentation modes.
        {
            u32 pm_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &pm_count, nullptr);

            // TODO: This is bad, we are possibly leaking memory.
            swc_info.presentation_modes.init(arena, pm_count);
            mina_vk_assert(vkGetPhysicalDeviceSurfacePresentModesKHR(
                pdev,
                surf,
                &pm_count,
                swc_info.presentation_modes.buf));
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surf, &swc_info.surface_capabilities);
    }

    void create_swap_chain(
        VkDevice             dev,
        VkSurfaceKHR         surf,
        SwapChain&           swc,
        QueueFamilies const& queues,
        WindowHandle*        win_handle,
        SwapChainInfo const& swc_info) noexcept {
        // Select optimal surface format.
        u32 optimal_surf_fmt_idx = 0;
        for (u32 idx = 0; idx < swc_info.surface_formats.size; ++idx) {
            bool const good_fmt = (swc_info.surface_formats[idx].format == VK_FORMAT_B8G8R8A8_SRGB);
            bool const good_col_space =
                (swc_info.surface_formats[idx].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

            if (good_fmt && good_col_space) {
                optimal_surf_fmt_idx = idx;
                break;
            }
        }
        swc.surface_format = swc_info.surface_formats[optimal_surf_fmt_idx];

        // Select optimal extent.
        if (swc_info.surface_capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
            swc.extent.width  = swc_info.surface_capabilities.currentExtent.width;
            swc.extent.height = swc_info.surface_capabilities.currentExtent.height;
        } else {
            i32 fb_width, fb_height;
            glfwGetFramebufferSize(win_handle, &fb_width, &fb_height);

            swc.extent.width = psh_clamp(
                static_cast<u32>(fb_width),
                swc_info.surface_capabilities.minImageExtent.width,
                swc_info.surface_capabilities.maxImageExtent.width);
            swc.extent.height = psh_clamp(
                static_cast<u32>(fb_height),
                swc_info.surface_capabilities.minImageExtent.height,
                swc_info.surface_capabilities.maxImageExtent.height);
        }

        // Compute the number of images handled by the swap chain. Whenever the maximum image count
        // is non-zero, there exists a limit to be accounted.
        u32 img_count = (swc_info.surface_capabilities.maxImageCount == 0)
                            ? (swc_info.surface_capabilities.minImageCount + 1)
                            : psh_min(
                                  swc_info.surface_capabilities.minImageCount + 1,
                                  swc_info.surface_capabilities.maxImageCount);

        swc.max_frames_in_flight = img_count - 1;

        VkSwapchainCreateInfoKHR create_info{
            .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface          = surf,
            .minImageCount    = img_count,
            .imageFormat      = swc.surface_format.format,
            .imageColorSpace  = swc.surface_format.colorSpace,
            .imageExtent      = swc.extent,
            .imageArrayLayers = 1,
            .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            // No transform to the image.
            .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            // No alpha-compose with other windows.
            .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            // NOTE: Although mailbox may be better for performance reasons, it may cause the screen
            //       region that got reduced to be completely black, as if we just erased that part
            //       from the frame buffer.
            .presentMode      = VK_PRESENT_MODE_FIFO_KHR,
            // Ignore the color of pixels outside the view.
            .clipped          = VK_TRUE,
            // NOTE: We should start dealing with old swap chains as soon as we want
            //       resizing windows.
            .oldSwapchain     = nullptr,
        };

        // Resolve the sharing mode for the images.
        if (queues.graphics_queue_index != queues.present_queue_index) {
            psh::FatPtr<u32 const> indices    = queues.indices();
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = static_cast<u32>(indices.size);
            create_info.pQueueFamilyIndices   = indices.buf;
        } else {
            // In the exclusive case we have to deal with ownership transfers of the images
            // between queue families via image memory barriers.
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        mina_vk_assert(vkCreateSwapchainKHR(dev, &create_info, nullptr, &swc.handle));
    }

    void destroy_swap_chain(VkDevice dev, SwapChain& swc) noexcept {
        usize img_count = swc.image_views.size;
        for (usize idx = 0; idx < img_count; ++idx) {
            vkDestroyImageView(dev, swc.image_views[idx], nullptr);
        }
        for (usize idx = 0; idx < img_count; ++idx) {
            vkDestroyFramebuffer(dev, swc.frame_bufs[idx], nullptr);
        }
        vkDestroySwapchainKHR(dev, swc.handle, nullptr);
    }

    void create_image_views(VkDevice dev, SwapChain& swc, psh::Arena* persistent_arena) noexcept {
        u32 img_count;
        vkGetSwapchainImagesKHR(dev, swc.handle, &img_count, nullptr);

        swc.images.init(persistent_arena, img_count);
        swc.image_views.init(persistent_arena, img_count);

        vkGetSwapchainImagesKHR(dev, swc.handle, &img_count, swc.images.buf);

        VkImageViewCreateInfo img_info{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = swc.surface_format.format,
            .components       = IMAGE_COMPONENT_MAPPING,
            .subresourceRange = IMAGE_SUBRESOURCE_RANGE,
        };

        for (u32 idx = 0; idx < img_count; ++idx) {
            img_info.image = swc.images[idx];
            mina_vk_assert(vkCreateImageView(dev, &img_info, nullptr, &swc.image_views[idx]));
        }
    }

    void create_frame_buffers(
        VkDevice          dev,
        SwapChain&        swc,
        psh::Arena*       persistent_arena,
        RenderPass const& gfx_pass) noexcept {
        usize img_count = swc.image_views.size;
        swc.frame_bufs.init(persistent_arena, img_count);

        VkFramebufferCreateInfo fb_info{
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = gfx_pass.handle,
            .attachmentCount = 1,
            .width           = swc.extent.width,
            .height          = swc.extent.height,
            .layers          = 1,
        };

        for (u32 idx = 0; idx < img_count; ++idx) {
            fb_info.pAttachments = &swc.image_views[idx];
            mina_vk_assert(vkCreateFramebuffer(dev, &fb_info, nullptr, &swc.frame_bufs[idx]));
        }
    }

    FrameStatus
    prepare_frame_for_rendering(VkDevice dev, SwapChain& swc, FrameResources& resources) noexcept {
        VkResult fence_status = vkGetFenceStatus(dev, resources.frame_in_flight_fence);
        switch (fence_status) {
            case VK_SUCCESS:           break;
            case VK_NOT_READY:         return FrameStatus::NOT_READY;
            case VK_ERROR_DEVICE_LOST: psh_todo_msg("Handle device lost");  // TODO: error handling.
            default:                   psh_unreachable();
        }

        constexpr u64 NEXT_IMAGE_TIMEOUT = UINT64_MAX;
        VkResult      img_res            = vkAcquireNextImageKHR(
            dev,
            swc.handle,
            NEXT_IMAGE_TIMEOUT,
            resources.image_available_semaphore,
            nullptr,
            &swc.current_image_index);

        FrameStatus status = FrameStatus::OK;
        switch (img_res) {
            case VK_SUCCESS: {
                // Set the swap-chain related resources.
                resources.image     = swc.images[swc.current_image_index];
                resources.frame_buf = swc.frame_bufs[swc.current_image_index];

                mina_vk_assert(vkResetFences(dev, 1, &resources.frame_in_flight_fence));
                break;
            }
            case VK_SUBOPTIMAL_KHR:        // Pass-through.
            case VK_ERROR_OUT_OF_DATE_KHR: status = FrameStatus::SWAP_CHAIN_OUT_OF_DATE; break;
            case VK_TIMEOUT:               // Pass-through.
            case VK_NOT_READY:             status = FrameStatus::NOT_READY; break;
            default:                       psh_unreachable();
        }

        return status;
    }

    PresentStatus present_frame(
        SwapChain&    swc,
        Window const& win,
        VkQueue       present_queue,
        VkSemaphore   finished_gfx_pass) noexcept {
        VkPresentInfoKHR present_info{
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &finished_gfx_pass,
            .swapchainCount     = 1,
            .pSwapchains        = &swc.handle,
            .pImageIndices      = &swc.current_image_index,
        };
        VkResult res = vkQueuePresentKHR(present_queue, &present_info);

        PresentStatus present_status = {};
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || win.resized) {
            present_status = PresentStatus::SWAP_CHAIN_OUT_OF_DATE;
        } else if (res != VK_SUCCESS) {
            // TODO: proper error handling.
            psh_todo_msg("Handle presentation fail.");
            present_status = PresentStatus::FATAL;
        }

        // Switch to the next available frame.
        swc.current_frame = (swc.current_frame + 1u) % swc.max_frames_in_flight;

        return present_status;
    }
}  // namespace mina
