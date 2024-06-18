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
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/gfx/sync.h>

#include <mina/gfx/utils.h>
#include <vulkan/vulkan_core.h>

namespace mina {
    void create_synchronizers(
        VkDevice             dev,
        SynchronizerManager& sync,
        psh::Arena*          persistent_arena,
        u32                  max_frames_in_flight) noexcept {
        // Initialize the synchronization objects.
        {
            // The number of synchronizers should match the maximum number of frames in flight.
            sync.frame_in_flight.frame_fence.init(persistent_arena, max_frames_in_flight);
            sync.finished_transfer.frame_fence.init(persistent_arena, max_frames_in_flight);
            sync.image_available.frame_semaphore.init(persistent_arena, max_frames_in_flight);
            sync.finished_render_pass.frame_semaphore.init(persistent_arena, max_frames_in_flight);
        }

        // Create the synchronization objects.
        {
            VkFenceCreateInfo fif_fence_info{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                // NOTE: The fence will be created signaled so that the first iteration of the main
                // loop
                //       isn't blocked by the call to `vkWaitForFences`.
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
            VkFenceCreateInfo     transf_fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

            for (VkFence& fif_fence : sync.frame_in_flight.frame_fence) {
                mina_vk_assert(vkCreateFence(dev, &fif_fence_info, nullptr, &fif_fence));
            }
            for (VkFence& transf_fence : sync.finished_transfer.frame_fence) {
                mina_vk_assert(vkCreateFence(dev, &transf_fence_info, nullptr, &transf_fence));
            }
            for (VkSemaphore& swc_sema : sync.image_available.frame_semaphore) {
                mina_vk_assert(vkCreateSemaphore(dev, &semaphore_info, nullptr, &swc_sema));
            }
            for (VkSemaphore& render_pass_sema : sync.finished_render_pass.frame_semaphore) {
                mina_vk_assert(vkCreateSemaphore(dev, &semaphore_info, nullptr, &render_pass_sema));
            }
        }
    }

    void destroy_synchronizers(VkDevice dev, SynchronizerManager& sync) noexcept {
        for (VkFence fif_fence : sync.frame_in_flight.frame_fence) {
            vkDestroyFence(dev, fif_fence, nullptr);
        }
        for (VkFence transf_fence : sync.finished_transfer.frame_fence) {
            vkDestroyFence(dev, transf_fence, nullptr);
        }
        for (VkSemaphore swc_sema : sync.image_available.frame_semaphore) {
            vkDestroySemaphore(dev, swc_sema, nullptr);
        }
        for (VkSemaphore render_pass_sema : sync.finished_render_pass.frame_semaphore) {
            vkDestroySemaphore(dev, render_pass_sema, nullptr);
        }
    }
}  // namespace mina
