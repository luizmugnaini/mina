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
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/swap_chain.h>

#include <mina/gfx/utils.h>
#include <psh/buffer.h>
#include <psh/intrinsics.h>
#include <psh/mem_utils.h>
#include <vulkan/vulkan_core.h>
#include <cstring>

namespace mina::gfx {
    namespace {
        constexpr u64 NEXT_IMAGE_TIMEOUT = UINT64_MAX;

        // Does the same as `create_image_views` but without initializing the image and image view
        // arrays, simply reusing and overriding their memory.
        void recreate_image_views(GraphicsContext& ctx) noexcept {
            u32 img_count = static_cast<u32>(ctx.swap_chain.img.size);

            vkGetSwapchainImagesKHR(
                ctx.dev,
                ctx.swap_chain.handle,
                &img_count,
                ctx.swap_chain.img.buf);

            VkImageViewCreateInfo img_info{
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = ctx.swap_chain.surf_fmt.format,
                .components =
                    VkComponentMapping{
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
            };

            for (u32 idx = 0; idx < img_count; ++idx) {
                img_info.image = ctx.swap_chain.img[idx];
                mina_vk_assert(
                    vkCreateImageView(ctx.dev, &img_info, nullptr, &ctx.swap_chain.img_view[idx]));
            }
        }

        // NOTE: Does the same as `create_frame_buffers` but without initializing the frame buffer
        // array,
        //       simply reusing and overriding its memory.
        void recreate_frame_buffers(GraphicsContext& ctx) noexcept {
            usize const img_count = ctx.swap_chain.img_view.size;
            ctx.swap_chain.frame_buf.init(ctx.persistent_arena, img_count);

            VkFramebufferCreateInfo fb_info{
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = ctx.pipelines.graphics.pass.handle,
                .attachmentCount = 1,
                .width           = ctx.swap_chain.extent.width,
                .height          = ctx.swap_chain.extent.height,
                .layers          = 1,
            };

            for (u32 idx = 0; idx < img_count; ++idx) {
                fb_info.pAttachments = &ctx.swap_chain.img_view[idx];
                mina_vk_assert(vkCreateFramebuffer(
                    ctx.dev,
                    &fb_info,
                    nullptr,
                    &ctx.swap_chain.frame_buf[idx]));
            }
        }

        void recreate_swap_chain(GraphicsContext& ctx) noexcept {
            ctx.window.wait_if_minimized();

            // Complete all operations before proceeding
            vkDeviceWaitIdle(ctx.dev);

            destroy_swap_chain(ctx);
            {
                auto          sarena = ctx.work_arena->make_scratch();
                SwapChainInfo swc_info;
                query_swap_chain_info(sarena.arena, ctx.pdev, ctx.surf, swc_info);

                // NOTE: since the swap chain creation doesn't require persistent arena memory, we
                // can
                //       simply create it again (no need for a "recreate" function).
                create_swap_chain(ctx, swc_info);
            }
            recreate_image_views(ctx);
            recreate_frame_buffers(ctx);
        }

    }  // namespace

    void query_swap_chain_info(
        psh::Arena*      arena,
        VkPhysicalDevice pdev,
        VkSurfaceKHR     surf,
        SwapChainInfo&   swc_info) noexcept {
        // Clear all previous queries.
        std::memset(reinterpret_cast<void*>(&swc_info), 0, sizeof(SwapChainInfo));

        // Surface formats.
        {
            u32 fmt_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &fmt_count, nullptr);

            swc_info.surf_fmt.init(arena, fmt_count);
            mina_vk_assert(vkGetPhysicalDeviceSurfaceFormatsKHR(
                pdev,
                surf,
                &fmt_count,
                swc_info.surf_fmt.buf));
        }

        // Presentation modes.
        {
            u32 pm_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &pm_count, nullptr);

            swc_info.pres_modes.init(arena, pm_count);
            mina_vk_assert(vkGetPhysicalDeviceSurfacePresentModesKHR(
                pdev,
                surf,
                &pm_count,
                swc_info.pres_modes.buf));
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surf, &swc_info.surf_capa);
    }

    void create_swap_chain(GraphicsContext& ctx, SwapChainInfo const& swc_info) noexcept {
        // Select optimal surface format.
        u32 optimal_surf_fmt_idx = 0;
        for (u32 idx = 0; idx < swc_info.surf_fmt.size; ++idx) {
            bool const good_fmt = (swc_info.surf_fmt[idx].format == VK_FORMAT_B8G8R8A8_SRGB);
            bool const good_col_space =
                (swc_info.surf_fmt[idx].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

            if (good_fmt && good_col_space) {
                optimal_surf_fmt_idx = idx;
                break;
            }
        }
        ctx.swap_chain.surf_fmt = swc_info.surf_fmt[optimal_surf_fmt_idx];

        // Select optimal extent.
        if (swc_info.surf_capa.currentExtent.width != std::numeric_limits<u32>::max()) {
            ctx.swap_chain.extent.width  = swc_info.surf_capa.currentExtent.width;
            ctx.swap_chain.extent.height = swc_info.surf_capa.currentExtent.height;
        } else {
            i32 fb_width, fb_height;
            glfwGetFramebufferSize(ctx.window.handle, &fb_width, &fb_height);

            ctx.swap_chain.extent.width = psh_clamp(
                static_cast<u32>(fb_width),
                swc_info.surf_capa.minImageExtent.width,
                swc_info.surf_capa.maxImageExtent.width);
            ctx.swap_chain.extent.height = psh_clamp(
                static_cast<u32>(fb_height),
                swc_info.surf_capa.minImageExtent.height,
                swc_info.surf_capa.maxImageExtent.height);
        }

        // Select optimal present mode.
        //
        // NOTE: Although mailbox may be better for performance reasons, it may cause the screen
        //       region that got reduced to be completely black, as if we just erased that part from
        //       the frame buffer.
        VkPresentModeKHR optimal_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        // if (psh::contains(VK_PRESENT_MODE_MAILBOX_KHR, swc_info.pres_modes.const_fat_ptr())) {
        //     optimal_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        // }

        // Compute the number of images handled by the swap chain.
        u32 const max_img_count = swc_info.surf_capa.maxImageCount;
        u32       img_count     = swc_info.surf_capa.minImageCount + 1;
        if (max_img_count != 0) {  // There is a restricted maximum.
            img_count = psh_min(img_count, max_img_count);
        }

        ctx.swap_chain.max_frames_in_flight = img_count - 1;

        VkSwapchainCreateInfoKHR create_info{
            .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface          = ctx.surf,
            .minImageCount    = img_count,
            .imageFormat      = ctx.swap_chain.surf_fmt.format,
            .imageColorSpace  = ctx.swap_chain.surf_fmt.colorSpace,
            .imageExtent      = ctx.swap_chain.extent,
            .imageArrayLayers = 1,
            .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            // No transform to the image.
            .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            // No alpha-compose with other windows.
            .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            // TODO: change this to mailbox in the future.
            .presentMode      = optimal_present_mode,
            // Ignore the color of pixels outside the view.
            .clipped          = VK_TRUE,
            // NOTE: We should start dealing with old swap chains as soon as we want
            //       resizing windows.
            .oldSwapchain     = nullptr,
        };

        auto               sarena           = ctx.work_arena->make_scratch();
        psh::DynArray<u32> queue_uidx       = ctx.queues.unique_indices(sarena.arena);
        usize const        unique_idx_count = queue_uidx.size;
        if (unique_idx_count != 1) {
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = static_cast<u32>(unique_idx_count);
            create_info.pQueueFamilyIndices   = queue_uidx.buf;
        } else {
            // In the exclusive case we have to deal with ownership transfers of the images
            // between queue families.
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        mina_vk_assert(
            vkCreateSwapchainKHR(ctx.dev, &create_info, nullptr, &ctx.swap_chain.handle));
    }

    void create_image_views(GraphicsContext& ctx) noexcept {
        u32 img_count;
        vkGetSwapchainImagesKHR(ctx.dev, ctx.swap_chain.handle, &img_count, nullptr);

        ctx.swap_chain.img.init(ctx.persistent_arena, img_count);
        ctx.swap_chain.img_view.init(ctx.persistent_arena, img_count);

        vkGetSwapchainImagesKHR(ctx.dev, ctx.swap_chain.handle, &img_count, ctx.swap_chain.img.buf);

        VkImageViewCreateInfo img_info{
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = ctx.swap_chain.surf_fmt.format,
            .components =
                VkComponentMapping{
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
        };

        for (u32 idx = 0; idx < img_count; ++idx) {
            img_info.image = ctx.swap_chain.img[idx];
            mina_vk_assert(
                vkCreateImageView(ctx.dev, &img_info, nullptr, &ctx.swap_chain.img_view[idx]));
        }
    }

    void create_frame_buffers(GraphicsContext& ctx) noexcept {
        usize const img_count = ctx.swap_chain.img_view.size;
        ctx.swap_chain.frame_buf.init(ctx.persistent_arena, img_count);

        VkFramebufferCreateInfo fb_info{
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = ctx.pipelines.graphics.pass.handle,
            .attachmentCount = 1,
            .width           = ctx.swap_chain.extent.width,
            .height          = ctx.swap_chain.extent.height,
            .layers          = 1,
        };

        for (u32 idx = 0; idx < img_count; ++idx) {
            fb_info.pAttachments = &ctx.swap_chain.img_view[idx];
            mina_vk_assert(
                vkCreateFramebuffer(ctx.dev, &fb_info, nullptr, &ctx.swap_chain.frame_buf[idx]));
        }
    }

    bool prepare_frame_for_rendering(GraphicsContext& ctx) noexcept {
        u32 const current_frame = ctx.swap_chain.current_frame;

        // TODO: We should possibly use vkGetFenceStatus and return false if the fence isn't
        // signaled.
        //       This allows us to continue working instead of stalling.
        VkResult const fence_status =
            vkGetFenceStatus(ctx.dev, ctx.sync.frame_in_flight[current_frame]);
        switch (fence_status) {
            case VK_SUCCESS:           break;
            case VK_NOT_READY:         return false;
            case VK_ERROR_DEVICE_LOST: psh_todo();
            default:                   psh_unreachable();
        }

        VkResult const img_res = vkAcquireNextImageKHR(
            ctx.dev,
            ctx.swap_chain.handle,
            NEXT_IMAGE_TIMEOUT,
            ctx.sync.swap_chain[current_frame],
            nullptr,
            &ctx.swap_chain.current_img_idx);

        bool success = true;
        switch (img_res) {
            case VK_SUCCESS:               break;
            case VK_SUBOPTIMAL_KHR:                                   // Pass-through.
            case VK_ERROR_OUT_OF_DATE_KHR: recreate_swap_chain(ctx);  // Pass-through.
            case VK_TIMEOUT:                                          // Pass-through.
            case VK_NOT_READY:             success = false; break;
            default:                       psh_unreachable();
        }

        // Late reset to avoid deadlocks.
        if (psh_likely(success)) {
            mina_vk_assert(vkResetFences(ctx.dev, 1, &ctx.sync.frame_in_flight[current_frame]));
        }

        return success;
    }

    bool present_frame(GraphicsContext& ctx) noexcept {
        VkPresentInfoKHR present_info{
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &ctx.sync.render_pass[ctx.swap_chain.current_frame],
            .swapchainCount     = 1,
            .pSwapchains        = &ctx.swap_chain.handle,
            .pImageIndices      = &ctx.swap_chain.current_img_idx,
        };
        VkResult res = vkQueuePresentKHR(ctx.queues.present, &present_info);

        bool success = true;
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || ctx.window.resized) {
            recreate_swap_chain(ctx);
        } else if (res != VK_SUCCESS) {
            // TODO: proper error handling.
            log_fmt(psh::LogLevel::Error, "Unable to present swap chain image");
            success = false;
        }

        // Switch to the next available frame.
        ctx.swap_chain.current_frame =
            (ctx.swap_chain.current_frame + 1u) % ctx.swap_chain.max_frames_in_flight;

        return success;
    }

    void destroy_swap_chain(GraphicsContext& ctx) noexcept {
        usize const img_count = ctx.swap_chain.img_view.size;
        for (usize idx = 0; idx < img_count; ++idx) {
            vkDestroyImageView(ctx.dev, ctx.swap_chain.img_view[idx], nullptr);
        }
        for (usize idx = 0; idx < img_count; ++idx) {
            vkDestroyFramebuffer(ctx.dev, ctx.swap_chain.frame_buf[idx], nullptr);
        }
        vkDestroySwapchainKHR(ctx.dev, ctx.swap_chain.handle, nullptr);
    }
}  // namespace mina::gfx
