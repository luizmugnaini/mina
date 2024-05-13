/// Author: Luiz Gustavo Mugnaini Anselmo <luizmugnaini@gmail.com>
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
/// Description: Vulkan graphics application context.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/gfx/types.h>
#include <mina/gfx/window.h>
#include <psh/arena.h>
#include <psh/assert.h>
#include <psh/dyn_array.h>
#include <psh/types.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    struct GraphicsContextGPUBufferConfig {
        usize staging_buffer_size;
        usize data_buffer_size;
    };

    struct GraphicsContextConfig {
        psh::Arena*                    persistent_arena;
        psh::Arena*                    work_arena;
        WindowConfig                   win_config;
        GraphicsContextGPUBufferConfig buf_config;
    };

    struct GraphicsContext {
        psh::Arena*       persistent_arena = nullptr;
        psh::Arena*       work_arena       = nullptr;
        psh::DynArray<u8> data{};
        Window            window{};
        VkInstance        instance = nullptr;
        VkPhysicalDevice  pdev     = nullptr;
        VkSurfaceKHR      surf     = nullptr;
        VkDevice          dev      = nullptr;
        VmaAllocator      gpu_allocator{};
        GPUBuffers        buffers{};
        SwapChain         swap_chain{};
        QueueFamilies     queues{};
        Pipelines         pipelines{};
        Commands          cmd{};
        Synchronizers     sync{};

#if defined(MINA_VULKAN_DEBUG)
        VkDebugUtilsMessengerEXT dbg_msg = nullptr;
#endif
    };

    /// Initialize the graphics context instance according to the given configurations.
    void init_graphics_context(GraphicsContext& ctx, GraphicsContextConfig const& config) noexcept;

    /// Destroy all resources attached and managed by the graphics context.
    void destroy_graphics_context(GraphicsContext& ctx) noexcept;
}  // namespace mina::gfx
