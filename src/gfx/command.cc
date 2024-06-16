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
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/command.h>

#include <mina/gfx/utils.h>
#include <psh/defer.h>

namespace mina::gfx {
    void create_command_buffers(GraphicsContext& ctx) {
        VkCommandPoolCreateInfo const cmd_pool_info{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            // Command buffers can be re-recorded individually.
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = ctx.queues.graphics_idx,
        };
        mina_vk_assert(vkCreateCommandPool(ctx.dev, &cmd_pool_info, nullptr, &ctx.cmd.pool));

        VkCommandBufferAllocateInfo const cmd_buf_alloc_info{
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = ctx.cmd.pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        mina_vk_assert(vkAllocateCommandBuffers(ctx.dev, &cmd_buf_alloc_info, ctx.cmd.gfx_buf.buf));
    }

    void record_command_buffers(GraphicsContext& ctx) {
        VkCommandBuffer buf = ctx.cmd.gfx_buf[ctx.swap_chain.current_frame];

        VkCommandBufferBeginInfo const begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        mina_vk_assert(vkBeginCommandBuffer(buf, &begin_info));
        {
            VkClearValue const          clear_color{.color = {.float32{MINA_CLEAR_COLOR}}};
            VkRenderPassBeginInfo const pass_begin_info{
                .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass  = ctx.pipelines.graphics.pass.handle,
                .framebuffer = ctx.swap_chain.frame_buf[ctx.swap_chain.current_img_idx],
                .renderArea{
                    .offset = {0, 0},
                    .extent = ctx.swap_chain.extent,
                },
                .clearValueCount = 1,
                .pClearValues    = &clear_color,
            };

            vkCmdBeginRenderPass(buf, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(
                    buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ctx.pipelines.graphics.handle);

                VkViewport const dyn_viewport{
                    .x        = 0.0f,
                    .y        = 0.0f,
                    .width    = static_cast<f32>(ctx.swap_chain.extent.width),
                    .height   = static_cast<f32>(ctx.swap_chain.extent.height),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                };
                vkCmdSetViewport(buf, 0, 1, &dyn_viewport);

                VkRect2D const dyn_scissors{
                    .offset = {0, 0},
                    .extent = ctx.swap_chain.extent,
                };
                vkCmdSetScissor(buf, 0, 1, &dyn_scissors);

                // TODO: this is completely hard coded. Remove this ASAP.
                constexpr u32 num_vertices  = 3;
                constexpr u32 num_instances = 1;
                vkCmdDraw(buf, num_vertices, num_instances, 0, 0);
            }
            vkCmdEndRenderPass(buf);
        }
        mina_vk_assert_msg(vkEndCommandBuffer(buf), "Failed to record commands to the buffer");
    }
}  // namespace mina::gfx
