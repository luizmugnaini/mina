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
/// Description: Implementation of the command buffer management.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/gfx/command.h>

#include <mina/gfx/utils.h>

namespace mina {
    // -----------------------------------------------------------------------------
    // - Implementation of the command buffer lifetime -
    // -----------------------------------------------------------------------------

    void create_command_buffers(
        VkDevice             dev,
        CommandManager&      commander,
        QueueFamilies const& queues,
        psh::Arena*          persistent_arena,
        u32                  max_frames_in_flight) noexcept {
        // Create command buffer pools.
        {
            VkCommandPoolCreateInfo cmd_pool_info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                // Command buffers will be short-lived - only submitted once and then re-recorded.
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                         VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queueFamilyIndex = queues.graphics_queue_index,
            };

            mina_vk_assert(vkCreateCommandPool(dev, &cmd_pool_info, nullptr, &commander.pool));
        }

        // Create command buffers.
        {
            // The number of graphics and transfer buffers should match the number of frames in
            // flight.
            commander.transfer.cmd.init(persistent_arena, max_frames_in_flight);
            commander.graphics.cmd.init(persistent_arena, max_frames_in_flight);

            VkCommandBufferAllocateInfo cmd_buf_alloc_info{
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = commander.pool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            for (VkCommandBuffer& gfx_cmd : commander.graphics.cmd) {
                mina_vk_assert(vkAllocateCommandBuffers(dev, &cmd_buf_alloc_info, &gfx_cmd));
            }
            for (VkCommandBuffer& transf_cmd : commander.transfer.cmd) {
                mina_vk_assert(vkAllocateCommandBuffers(dev, &cmd_buf_alloc_info, &transf_cmd));
            }
        }
    }

    void destroy_command_buffers(VkDevice dev, CommandManager& cmds) noexcept {
        vkDestroyCommandPool(dev, cmds.pool, nullptr);
    }

    // -----------------------------------------------------------------------------
    // - Implementation of the data transfer routines -
    // -----------------------------------------------------------------------------

    // NOTE: assumes that VBO-UBO are tightly packed
    TransferInfo transfer_whole_info(
        VkBuffer src,
        VkBuffer dst,
        usize    vertex_buf_size,
        usize    uniform_buf_size) noexcept {
        return TransferInfo{
            .src_buf_handle   = src,
            .dst_buf_handle   = dst,
            .src_buf_offset   = 0,
            .dst_buf_offset   = 0,
            .vertex_buf_size  = vertex_buf_size,
            .uniform_buf_size = uniform_buf_size,
        };
    }

    void record_transfer_commands(VkCommandBuffer transfer_cmd, TransferInfo const& info) noexcept {
        mina_vk_assert(vkResetCommandBuffer(transfer_cmd, 0));

        constexpr VkCommandBufferBeginInfo BEGIN_INFO{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,  // Submitted once per frame.
        };

        mina_vk_assert(vkBeginCommandBuffer(transfer_cmd, &BEGIN_INFO));
        {
            VkBufferCopy buf_copy_info{
                .srcOffset = info.src_buf_offset,
                .dstOffset = info.dst_buf_offset,
                .size      = info.vertex_buf_size + info.uniform_buf_size,
            };
            vkCmdCopyBuffer(
                transfer_cmd,
                info.src_buf_handle,
                info.dst_buf_handle,
                1,
                &buf_copy_info);

            usize vertex_buf_dst_offset = info.dst_buf_offset;
            // usize uniform_buf_dst_offset = info.dst_buf_offset + info.vertex_buf_size;
            //
            // // Prepare the uniform buffer memory for the vertex shader stage.
            // VkBufferMemoryBarrier transfer_uniform_buf_barrier{
            //     .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            //     .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            //     .dstAccessMask       = VK_ACCESS_UNIFORM_READ_BIT,
            //     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            //     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            //     .buffer              = info.dst_buf_handle,
            //     .offset              = uniform_buf_dst_offset,
            //     .size                = info.uniform_buf_size,
            // };
            // vkCmdPipelineBarrier(
            //     transfer_cmd,
            //     VK_PIPELINE_STAGE_TRANSFER_BIT,
            //     VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            //     0,
            //     0,
            //     nullptr,
            //     1,
            //     &transfer_uniform_buf_barrier,
            //     0,
            //     nullptr);

            // Prepare the vertex buffer memory to be read by the vertex shader.
            VkBufferMemoryBarrier transfer_vertex_buf_barrier{
                .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer              = info.dst_buf_handle,
                .offset              = vertex_buf_dst_offset,
                .size                = info.vertex_buf_size,
            };
            vkCmdPipelineBarrier(
                transfer_cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                0,
                0,
                nullptr,
                1,
                &transfer_vertex_buf_barrier,
                0,
                nullptr);
        }
        mina_vk_assert(vkEndCommandBuffer(transfer_cmd));
    }

    void submit_transfer_commands(
        VkQueue         graphics_queue,
        VkCommandBuffer transfer_cmd,
        VkFence         transfer_fence) noexcept {
        VkSubmitInfo submit_info{
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 0,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &transfer_cmd,
            .signalSemaphoreCount = 0,
        };
        mina_vk_assert(vkQueueSubmit(graphics_queue, 1, &submit_info, transfer_fence));
    }

    void wait_transfer_completion(VkDevice dev, VkFence transfer_fence) noexcept {
        constexpr u64 DATA_TRANSFER_TIMEOUT = UINT64_MAX;
        mina_vk_assert(vkWaitForFences(dev, 1, &transfer_fence, true, DATA_TRANSFER_TIMEOUT));
        mina_vk_assert(vkResetFences(dev, 1, &transfer_fence));
    }

    // -----------------------------------------------------------------------------
    // - Implementation of the graphics drawing routines -
    // -----------------------------------------------------------------------------

    void record_graphics_commands(
        VkCommandBuffer        graphics_cmd,
        QueueFamilies const&   queues,
        GraphicsCmdInfo const& info,
        RenderDataInfo const&  data_info) noexcept {
        mina_vk_assert(vkResetCommandBuffer(graphics_cmd, 0));

        constexpr VkCommandBufferBeginInfo BEGIN_INFO{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        mina_vk_assert(vkBeginCommandBuffer(graphics_cmd, &BEGIN_INFO));
        {
            bool const sharing_mode_is_exclusive =
                (queues.present_queue_index != queues.graphics_queue_index);

            if (sharing_mode_is_exclusive) {
                VkImageMemoryBarrier image_from_present_to_graphics_queue{
                    .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask    = VK_ACCESS_MEMORY_READ_BIT,
                    .dstAccessMask    = VK_ACCESS_MEMORY_READ_BIT,
                    .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .image            = info.image,
                    .subresourceRange = IMAGE_SUBRESOURCE_RANGE,
                };

                vkCmdPipelineBarrier(
                    graphics_cmd,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &image_from_present_to_graphics_queue);
            }

            constexpr VkClearValue clear_color{.color = {.float32{MINA_CLEAR_COLOR}}};
            VkRenderPassBeginInfo  render_pass_begin_info{
                 .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                 .renderPass  = info.render_pass.handle,
                 .framebuffer = info.frame_buf,
                 .renderArea{
                     .offset = {0, 0},
                     .extent = info.surface_extent,
                },
                 .clearValueCount = 1,
                 .pClearValues    = &clear_color,
            };

            vkCmdBeginRenderPass(graphics_cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(graphics_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);

                // Set the dynamic states.
                {
                    VkViewport dyn_viewport{
                        .x        = 0.0f,
                        .y        = 0.0f,
                        .width    = static_cast<f32>(info.surface_extent.width),
                        .height   = static_cast<f32>(info.surface_extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f,
                    };
                    vkCmdSetViewport(graphics_cmd, 0, 1, &dyn_viewport);

                    VkRect2D dyn_scissors{
                        .offset = {0, 0},
                        .extent = info.surface_extent,
                    };
                    vkCmdSetScissor(graphics_cmd, 0, 1, &dyn_scissors);
                }

                vkCmdBindDescriptorSets(
                    graphics_cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    info.pipeline_layout,
                    0,
                    1,
                    &info.uniform_buf_descriptor_set,
                    0,
                    nullptr);
                vkCmdBindVertexBuffers(
                    graphics_cmd,
                    info.vertex_buf_binding,
                    1,
                    &info.vertex_buf,
                    &data_info.offset);

                vkCmdDraw(
                    graphics_cmd,
                    data_info.vertex_count,
                    data_info.instance_count,
                    data_info.first_vertex_index,
                    data_info.first_instance_index);
            }
            vkCmdEndRenderPass(graphics_cmd);

            if (sharing_mode_is_exclusive) {
                VkImageMemoryBarrier image_from_graphics_to_present_queue{
                    .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask    = VK_ACCESS_MEMORY_READ_BIT,
                    .dstAccessMask    = VK_ACCESS_MEMORY_READ_BIT,
                    .oldLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .image            = info.image,
                    .subresourceRange = IMAGE_SUBRESOURCE_RANGE,
                };

                vkCmdPipelineBarrier(
                    graphics_cmd,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &image_from_graphics_to_present_queue);
            }
        }
        mina_vk_assert_msg(
            vkEndCommandBuffer(graphics_cmd),
            "Failed to record commands to the buffer");
    }

    void submit_graphics_commands(
        VkQueue         graphics_queue,
        VkCommandBuffer graphics_cmd,
        VkSemaphore     image_available,
        VkSemaphore     finished_render_pass,
        VkFence         frame_in_flight) noexcept {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo         submit_info{
                    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .waitSemaphoreCount   = 1,
                    .pWaitSemaphores      = &image_available,
                    .pWaitDstStageMask    = &wait_stage,
                    .commandBufferCount   = 1,
                    .pCommandBuffers      = &graphics_cmd,
                    .signalSemaphoreCount = 1,
                    .pSignalSemaphores    = &finished_render_pass,
        };

        mina_vk_assert_msg(
            vkQueueSubmit(graphics_queue, 1, &submit_info, frame_in_flight),
            "Unable to submit draw commands to the graphics queue");
    }
}  // namespace mina
