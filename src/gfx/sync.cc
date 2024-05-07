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
/// Description: Implementation of the Vulkan graphics synchronization layer.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/sync.h>

#include <mina/gfx/utils.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    void create_synchronizers(GraphicsContext& ctx) noexcept {
        // The number of synchronizers should match the maximum number of frames in flight.
        u32 sync_count = ctx.swap_chain.max_frames_in_flight;

        ctx.sync.frame_in_flight.init(ctx.persistent_arena, sync_count);
        ctx.sync.swap_chain.init(ctx.persistent_arena, sync_count);
        ctx.sync.render_pass.init(ctx.persistent_arena, sync_count);

        // NOTE: The fence will be created signaled so that the first iteration of the main loop
        //       isn't blocked by the call to `vkWaitForFences`.
        VkFenceCreateInfo const fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        for (auto& fiff : ctx.sync.frame_in_flight) {
            mina_vk_assert(vkCreateFence(ctx.dev, &fence_info, nullptr, &fiff));
        }

        VkSemaphoreCreateInfo const semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        for (auto& swcs : ctx.sync.swap_chain) {
            mina_vk_assert(vkCreateSemaphore(ctx.dev, &semaphore_info, nullptr, &swcs));
        }
        for (auto& rps : ctx.sync.render_pass) {
            mina_vk_assert(vkCreateSemaphore(ctx.dev, &semaphore_info, nullptr, &rps));
        }
    }

    void destroy_synchronizers(GraphicsContext& ctx) noexcept {
        for (auto fiff : ctx.sync.frame_in_flight) {
            vkDestroyFence(ctx.dev, fiff, nullptr);
        }
        for (auto swcs : ctx.sync.swap_chain) {
            vkDestroySemaphore(ctx.dev, swcs, nullptr);
        }
        for (auto rps : ctx.sync.render_pass) {
            vkDestroySemaphore(ctx.dev, rps, nullptr);
        }
    }
}  // namespace mina::gfx
