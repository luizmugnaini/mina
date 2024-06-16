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
/// Description: Implementation of the Vulkan graphics pipeline management layer.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gfx/pipeline.h>

#include <mina/gfx/utils.h>
#include <psh/buffer.h>
#include <psh/defer.h>
#include <psh/file_system.h>
#include <psh/io.h>
#include <psh/string.h>

namespace mina::gfx {
    VkShaderModule make_shader_module(VkDevice dev, psh::StringView const& shader_src) noexcept {
        VkShaderModuleCreateInfo sm_info{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shader_src.data.size,
            .pCode    = reinterpret_cast<u32 const*>(shader_src.data.buf),
        };

        VkShaderModule sm;
        if (vkCreateShaderModule(dev, &sm_info, nullptr, &sm) != VK_SUCCESS) {
            sm = nullptr;
            psh_error_fmt(
                "Couldn't make shader module for shader:\n%s",
                shader_src.data.buf);
        }

        return sm;
    }

    // For the time being we only have a single subpass.
    void create_graphics_render_pass(GraphicsContext& ctx) noexcept {
        VkAttachmentDescription const color_att{
            .format        = ctx.swap_chain.surf_fmt.format,
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            // Make the frame-buffer black at the start.
            .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
            // Store the resulting frame-buffer in memory after rendering.
            .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
            // The initial layout doesn't matter since we are going to clear it.
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            // Prepare the image for presentation.
            .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        VkAttachmentReference const color_att_ref{
            // `color_att` will be the only attachment for the time being, so we use index zero.
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription const subpass_desc{
            .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &color_att_ref,
        };

        // Dependency on the last rendering subpass, barring the current subpass to work on the
        // pipeline stage only when the color att. output is reached (giving it write access to
        // the color att.)
        VkSubpassDependency const subpass_dep{
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        VkRenderPassCreateInfo const render_pass_info{
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &color_att,
            .subpassCount    = 1,
            .pSubpasses      = &subpass_desc,
            .dependencyCount = 1,
            .pDependencies   = &subpass_dep,
        };
        mina_vk_assert(vkCreateRenderPass(
            ctx.dev,
            &render_pass_info,
            nullptr,
            &ctx.pipelines.graphics.pass.handle));
    }

    void create_graphics_pipeline(GraphicsContext& ctx) noexcept {
        // Render pass associated to the immediate graphics pipeline.
        create_graphics_render_pass(ctx);

        auto sarena = ctx.work_arena->make_scratch();

        psh::FileReadResult vert =
            psh::read_file(sarena.arena, shader_path(ShaderCatalog::TRIANGLE_VERTEX), psh::ReadFileFlag::READ_BIN);
        psh::FileReadResult frag =
            psh::read_file(sarena.arena, shader_path(ShaderCatalog::TRIANGLE_FRAGMENT), psh::ReadFileFlag::READ_BIN);
        psh_assert(vert.status == psh::FileStatus::OK && frag.status == psh::FileStatus::OK);

        // Shader modules.
        psh::Buffer<VkShaderModule, 2> shaders{
            make_shader_module(ctx.dev, vert.content.view()),
            make_shader_module(ctx.dev, frag.content.view()),

        };

        // Stage information.
        psh::Buffer<VkPipelineShaderStageCreateInfo, 2> const pip_shader_stages_info{
            VkPipelineShaderStageCreateInfo{
                .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage  = VK_SHADER_STAGE_VERTEX_BIT,
                .module = shaders[0],
                .pName  = "main",
            },
            VkPipelineShaderStageCreateInfo{
                .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = shaders[1],
                .pName  = "main",
            },
        };

        // Pipeline layout for the constants accessed by the shader stages.
        VkPipelineLayoutCreateInfo const pip_layout_info{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 0,
            .pushConstantRangeCount = 0,
        };
        mina_vk_assert(vkCreatePipelineLayout(
            ctx.dev,
            &pip_layout_info,
            nullptr,
            &ctx.pipelines.graphics.layout));

        VkPipelineVertexInputStateCreateInfo const pip_vert_stage_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 0,
            .vertexAttributeDescriptionCount = 0,
        };

        VkPipelineInputAssemblyStateCreateInfo const pip_in_asm_state_info{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,  // NOTE: this disables index buffers
        };

        VkViewport const pip_viewport{
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = static_cast<float>(ctx.swap_chain.extent.width),
            .height   = static_cast<float>(ctx.swap_chain.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        VkRect2D const pip_scissors{
            .offset = {.x = 0, .y = 0},
            .extent = ctx.swap_chain.extent,
        };
        VkPipelineViewportStateCreateInfo const pip_viewport_state_info{
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports    = &pip_viewport,
            .scissorCount  = 1,
            .pScissors     = &pip_scissors,
        };

        VkPipelineRasterizationStateCreateInfo const pip_rast_state_info{
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .lineWidth               = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo const pip_sampling_state_info{
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable   = VK_FALSE,
            .minSampleShading      = 1.0f,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable      = VK_FALSE,
        };

        // Alpha-blending setup.
        VkPipelineColorBlendAttachmentState const pip_blend_attachment_state{
            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo const pip_blend_state_info{
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments    = &pip_blend_attachment_state,
        };

        psh::Buffer<VkDynamicState const, 2> const pip_dyn_states{
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_VIEWPORT,
        };
        VkPipelineDynamicStateCreateInfo const pip_dyn_states_info{
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = pip_dyn_states.size(),
            .pDynamicStates    = pip_dyn_states.buf,
        };

        VkGraphicsPipelineCreateInfo const pip_info{
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = static_cast<u32>(pip_shader_stages_info.size()),
            .pStages             = pip_shader_stages_info.buf,
            .pVertexInputState   = &pip_vert_stage_info,
            .pInputAssemblyState = &pip_in_asm_state_info,
            .pTessellationState  = nullptr,
            .pViewportState      = &pip_viewport_state_info,
            .pRasterizationState = &pip_rast_state_info,
            .pMultisampleState   = &pip_sampling_state_info,
            .pDepthStencilState  = nullptr,
            .pColorBlendState    = &pip_blend_state_info,
            .pDynamicState       = &pip_dyn_states_info,
            .layout              = ctx.pipelines.graphics.layout,
            .renderPass          = ctx.pipelines.graphics.pass.handle,
            .subpass             = 0,
        };
        mina_vk_assert(vkCreateGraphicsPipelines(
            ctx.dev,
            nullptr,
            1,
            &pip_info,
            nullptr,
            &ctx.pipelines.graphics.handle));

        for (VkShaderModule shader : shaders) {
            vkDestroyShaderModule(ctx.dev, shader, nullptr);
        }
    }

    void create_graphics_commands(GraphicsContext& ctx) noexcept {
        VkCommandPoolCreateInfo const cmd_pool_info{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            // Command buffers can be re-recorded individually.
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = ctx.queues.graphics_idx,
        };
        mina_vk_assert(vkCreateCommandPool(ctx.dev, &cmd_pool_info, nullptr, &ctx.cmd.pool));

        // The number of buffers should match the number of frames in flight.
        u32 buf_count = ctx.swap_chain.max_frames_in_flight;
        ctx.cmd.gfx_buf.init(ctx.persistent_arena, buf_count);

        VkCommandBufferAllocateInfo const cmd_buf_alloc_info{
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = ctx.cmd.pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        for (auto& buf : ctx.cmd.gfx_buf) {
            mina_vk_assert(vkAllocateCommandBuffers(ctx.dev, &cmd_buf_alloc_info, &buf));
        }
    }

    void record_graphics_command(GraphicsContext& ctx) noexcept {
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

                constexpr u32 num_vertices  = 3;
                constexpr u32 num_instances = 1;
                vkCmdDraw(buf, num_vertices, num_instances, 0, 0);
            }
            vkCmdEndRenderPass(buf);
        }
        mina_vk_assert_msg(vkEndCommandBuffer(buf), "Failed to record commands to the buffer");
    }

    void submit_graphics_commands(GraphicsContext& ctx) noexcept {
        u32             current_frame = ctx.swap_chain.current_frame;
        VkCommandBuffer gfx_buf       = ctx.cmd.gfx_buf[current_frame];

        mina_vk_assert(vkResetCommandBuffer(gfx_buf, 0));
        record_graphics_command(ctx);

        VkPipelineStageFlags const wait_stages[1]{
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
        VkSubmitInfo submit_info{
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &ctx.sync.swap_chain[current_frame],
            .pWaitDstStageMask    = wait_stages,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &gfx_buf,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &ctx.sync.render_pass[current_frame],
        };

        mina_vk_assert_msg(
            vkQueueSubmit(
                ctx.queues.graphics,
                1,
                &submit_info,
                ctx.sync.frame_in_flight[current_frame]),
            "Unable to submit draw commands to the graphics queue");
    }

    void destroy_graphics_pipeline(GraphicsContext& ctx) noexcept {
        vkDestroyPipeline(ctx.dev, ctx.pipelines.graphics.handle, nullptr);
        vkDestroyPipelineLayout(ctx.dev, ctx.pipelines.graphics.layout, nullptr);
        vkDestroyRenderPass(ctx.dev, ctx.pipelines.graphics.pass.handle, nullptr);
    }
}  // namespace mina::gfx
