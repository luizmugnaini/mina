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
/// Description: Implementation of the GPU buffer management layer.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/gpu_buffer.h>

#include <mina/gfx/context.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    void make_gpu_buffers(MakeGPUBuffersInfo const& buf_info, GraphicsContext& ctx) noexcept {
        // Data buffer - device local (GPU).
        {
            VkBufferCreateInfo data_info{
                .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext                 = nullptr,
                .flags                 = 0,
                .size                  = buf_info.staging_size,
                .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode           = VK_SHARING_MODE_CONCURRENT,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices   = &ctx.queues.transfer_idx,
            };
            VmaAllocationCreateInfo alloc_info{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
            };

            vmaCreateBuffer(
                ctx.gpu_allocator,
                &data_info,
                &alloc_info,
                &ctx.buffers.staging.handle,
                &ctx.buffers.staging.allocation,
                nullptr);
        }

        // Staging buffer - host local (CPU).
        {
            VkBufferCreateInfo staging_info{
                .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext                 = nullptr,
                .flags                 = 0,
                .size                  = buf_info.staging_size,
                .usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices   = &ctx.queues.transfer_idx,
            };
            VmaAllocationCreateInfo alloc_info{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
            };

            vmaCreateBuffer(
                ctx.gpu_allocator,
                &staging_info,
                &alloc_info,
                &ctx.buffers.staging.handle,
                &ctx.buffers.staging.allocation,
                nullptr);
        }
    }

    void transfer_host_data(GraphicsContext&) noexcept {
        psh_todo();
    }
}  // namespace mina::gfx
