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
/// Description: Vulkan graphics swap chain management layer.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/gfx/context.h>
#include <mina/window.h>
#include <psh/arena.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

namespace mina {
    void query_swap_chain_info(
        VkPhysicalDevice pdev,
        VkSurfaceKHR     surf,
        psh::Arena*      arena,
        SwapChainInfo&   swc_info) noexcept;

    void create_swap_chain(
        VkDevice             dev,
        VkSurfaceKHR         surf,
        SwapChain&           swc,
        QueueFamilies const& queues,
        WindowHandle*        win_handle,
        SwapChainInfo const& swc_info) noexcept;

    void destroy_swap_chain(VkDevice dev, SwapChain& swc) noexcept;

    void create_image_views(VkDevice dev, SwapChain& swc, psh::Arena* persistent_arena) noexcept;

    void recreate_image_views(VkDevice dev, SwapChain& swc) noexcept;

    void create_frame_buffers(
        VkDevice          dev,
        SwapChain&        swc,
        psh::Arena*       persistent_arena,
        RenderPass const& gfx_pass) noexcept;

    void recreate_frame_buffers(VkDevice dev, SwapChain& swc, RenderPass const& gfx_pass) noexcept;

    FrameStatus
    prepare_frame_for_rendering(VkDevice dev, SwapChain& swc, FrameResources& resources) noexcept;

    PresentStatus present_frame(
        SwapChain&    swc,
        Window const& win,
        VkQueue       present_queue,
        VkSemaphore   finished_gfx_pass) noexcept;

    inline f32 aspect_ratio(SwapChain& swc) {
        return static_cast<f32>(swc.extent.width / swc.extent.height);
    }
}  // namespace mina
