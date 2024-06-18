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
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/gfx/buffer.h>

#include <mina/gfx/utils.h>
#include <vulkan/vulkan_core.h>
#include <cstring>

namespace mina {
    void
    create_buffers(VmaAllocator alloc, BufferManager& buffers, QueueFamilies& queues) noexcept {
        // Host local (CPU) staging buffer.
        {
            buffers.host.size = HOST_BUFFER_SIZE;

            VkBufferCreateInfo host_buf_info{
                .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext                 = nullptr,
                .size                  = buffers.host.size,
                .usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices   = &queues.graphics_queue_index,
            };
            VmaAllocationCreateInfo host_alloc_info{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
            };

            mina_vk_assert(vmaCreateBuffer(
                alloc,
                &host_buf_info,
                &host_alloc_info,
                &buffers.host.handle,
                &buffers.host.allocation,
                nullptr));
        }

        // Device local buffer (GPU).
        {
            buffers.device.size = DEVICE_BUFFER_SIZE;

            VkBufferCreateInfo device_buf_info{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .size  = buffers.device.size,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };
            VmaAllocationCreateInfo device_alloc_info{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            };

            mina_vk_assert(vmaCreateBuffer(
                alloc,
                &device_buf_info,
                &device_alloc_info,
                &buffers.device.handle,
                &buffers.device.allocation,
                nullptr));
        }
    }

    void destroy_buffers(VmaAllocator alloc, BufferManager& bufs) noexcept {
        vmaDestroyBuffer(alloc, bufs.host.handle, bufs.host.allocation);
        vmaDestroyBuffer(alloc, bufs.device.handle, bufs.device.allocation);
    }

    void stage_host_data(
        VmaAllocator       alloc,
        Buffer const&      staging_buf,
        StagingInfo const& info) noexcept {
        void* staging_memory_ptr;
        mina_vk_assert(vmaMapMemory(alloc, staging_buf.allocation, &staging_memory_ptr));
        {
            u8 const* vbo_src = info.src_ptr + info.vertex_buf_src_offset;
            u8 const* ubo_src = info.src_ptr + info.uniform_buf_src_offset;
            u8* vbo_dst = reinterpret_cast<u8*>(staging_memory_ptr) + info.vertex_buf_dst_offset;
            u8* ubo_dst = reinterpret_cast<u8*>(staging_memory_ptr) + info.uniform_buf_dst_offset;

            // Check if the data to be staged is packed.
            bool host_data_is_packed   = (vbo_src + info.vertex_buf_size == ubo_src);
            // Check if the staging buffer will receive the host data in a packed manner.
            bool staging_dst_is_packed = (vbo_dst + info.vertex_buf_size == ubo_dst);

            if (host_data_is_packed && staging_dst_is_packed) {
                std::memcpy(vbo_dst, vbo_src, info.vertex_buf_size + info.uniform_buf_size);
            } else {
                std::memcpy(vbo_dst, vbo_src, info.vertex_buf_size);
                std::memcpy(ubo_dst, ubo_src, info.uniform_buf_size);
            }
        }
        vmaUnmapMemory(alloc, staging_buf.allocation);
    }
}  // namespace mina
