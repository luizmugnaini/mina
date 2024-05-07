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
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/gfx/context.h>
#include <psh/arena.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    void query_swap_chain_info(
        psh::Arena*      arena,
        VkPhysicalDevice pdev,
        VkSurfaceKHR     surf,
        SwapChainInfo&   swc_info) noexcept;

    void create_swap_chain(SwapChainInfo const& swc_info, GraphicsContext& ctx) noexcept;

    void create_image_views(GraphicsContext& ctx) noexcept;

    void create_frame_buffers(GraphicsContext& ctx) noexcept;

    [[nodiscard]] bool prepare_frame_for_rendering(GraphicsContext& ctx) noexcept;

    [[nodiscard]] bool present_frame(GraphicsContext& ctx) noexcept;

    void destroy_swap_chain(GraphicsContext& ctx) noexcept;
}  // namespace mina::gfx