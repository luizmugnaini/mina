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
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/gfx/pipeline.h>

#include <mina/gfx/data.h>
#include <mina/gfx/utils.h>
#include <psh/buffer.h>
#include <psh/defer.h>
#include <psh/streams.h>
#include <psh/log.h>
#include <psh/string.h>

namespace mina {
    // -----------------------------------------------------------------------------
    // - Internal implementation details -
    // -----------------------------------------------------------------------------

    namespace {
        VkShaderModule make_shader_module(
            VkDevice               dev,
            psh::StringView const& shader_src) noexcept {
            VkShaderModuleCreateInfo sm_info{
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = shader_src.data.size,
                .pCode    = reinterpret_cast<u32 const*>(shader_src.data.buf),
            };

            VkShaderModule sm;
            if (vkCreateShaderModule(dev, &sm_info, nullptr, &sm) != VK_SUCCESS) {
                sm = nullptr;
                psh_error_fmt("Couldn't make shader module for shader:\n%s", shader_src.data.buf);
            }
            return sm;
        }
    }  // namespace

    // -----------------------------------------------------------------------------
    // - Implementation of the descriptor set management -
    // -----------------------------------------------------------------------------

    void create_descriptor_sets(
        VkDevice              dev,
        DescriptorSetManager& descriptor_sets,
        Buffer const&         uniform_buf) noexcept {
        // Create the descriptor set layout.
        {
            constexpr VkDescriptorSetLayoutBinding UBO_LAYOUT_BINDING{
                .binding         = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT,
            };
            VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
                .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings    = &UBO_LAYOUT_BINDING,
            };
            mina_vk_assert(vkCreateDescriptorSetLayout(
                dev,
                &descriptor_set_layout_info,
                nullptr,
                &descriptor_sets.layout));
        }

        // Create the descriptor set pool.
        {
            VkDescriptorPoolSize pool_size{
                .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
            };
            VkDescriptorPoolCreateInfo pool_info{
                .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags         = 0,
                .maxSets       = 1,
                .poolSizeCount = 1,
                .pPoolSizes    = &pool_size,
            };

            mina_vk_assert(vkCreateDescriptorPool(dev, &pool_info, nullptr, &descriptor_sets.pool));
        }

        // Allocate the descriptor sets.
        {
            VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
                .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool     = descriptor_sets.pool,
                .descriptorSetCount = 1,
                .pSetLayouts        = &descriptor_sets.layout,
            };
            mina_vk_assert(vkAllocateDescriptorSets(
                dev,
                &descriptor_set_alloc_info,
                &descriptor_sets.uniform_buf_descriptor_set));
        }

        // Write the content of the descriptor set.
        {
            VkDescriptorBufferInfo ubo_info{
                .buffer = uniform_buf.handle,
                .offset = uniform_buf.offset,
                .range  = uniform_buf.size,
            };

            VkWriteDescriptorSet ubo_descriptor_write{
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = descriptor_sets.uniform_buf_descriptor_set,
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo     = &ubo_info,
            };

            vkUpdateDescriptorSets(dev, 1, &ubo_descriptor_write, 0, nullptr);
        }
    }

    void destroy_descriptor_sets(VkDevice dev, DescriptorSetManager& descriptor_sets) noexcept {
        vkDestroyDescriptorSetLayout(dev, descriptor_sets.layout, nullptr);
        vkDestroyDescriptorPool(dev, descriptor_sets.pool, nullptr);
    }

    // -----------------------------------------------------------------------------
    // - Implementation of the graphics pipeline lifetime management -
    // -----------------------------------------------------------------------------

    void create_graphics_pipeline_context(
        VkDevice              dev,
        psh::Arena*           persistent_arena,
        Pipeline&             graphics_pip,
        DescriptorSetManager& descriptor_sets,
        VkFormat              surf_fmt,
        VkExtent2D            surf_ext) noexcept {
        // Graphics render pass creation.
        {
            VkAttachmentDescription color_attachment{
                .format        = surf_fmt,
                .samples       = VK_SAMPLE_COUNT_1_BIT,
                // Make the frame-buffer black at the start.
                .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
                // Store the resulting frame-buffer in memory after rendering.
                .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            };
            VkAttachmentReference color_attachment_reference{
                // `color_att` will be the only attachment for the time being, so we use index zero.
                .attachment = 0,
                .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };
            VkSubpassDescription subpass_description{
                .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments    = &color_attachment_reference,
            };

            // Dependency barriers.
            psh::Buffer<VkSubpassDependency, 2> subpass_dependencies{
                // Transition from undefined to color attachment optimal.
                VkSubpassDependency{
                    .srcSubpass    = VK_SUBPASS_EXTERNAL,
                    .dstSubpass    = 0,
                    .srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT,  // Ensure prior work is done.
                    .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                },
                // Transition from color attachment optimal to present.
                VkSubpassDependency{
                    .srcSubpass    = 0,
                    .dstSubpass    = VK_SUBPASS_EXTERNAL,
                    .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                },
            };

            VkRenderPassCreateInfo render_pass_info{
                .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = 1,
                .pAttachments    = &color_attachment,
                .subpassCount    = 1,
                .pSubpasses      = &subpass_description,
                .dependencyCount = subpass_dependencies.size(),
                .pDependencies   = subpass_dependencies.buf,
            };
            mina_vk_assert(vkCreateRenderPass(
                dev,
                &render_pass_info,
                nullptr,
                &graphics_pip.render_pass.handle));
        }

        // Pipeline setup.
        {
            psh::ScratchArena sarena = persistent_arena->make_scratch();

            psh::FileReadResult vert = psh::read_file(
                sarena.arena,
                shader_path(ShaderCatalog::TRIANGLE_VERTEX),
                psh::ReadFileFlag::READ_BIN);
            psh_assert_msg(vert.status == psh::FileStatus::OK, "Failed to read vertex shader file");

            psh::FileReadResult frag = psh::read_file(
                sarena.arena,
                shader_path(ShaderCatalog::TRIANGLE_FRAGMENT),
                psh::ReadFileFlag::READ_BIN);
            psh_assert_msg(
                frag.status == psh::FileStatus::OK,
                "Failed to read fragment shader file");

            // Shader modules.
            psh::Buffer<VkShaderModule, 2> shaders{
                make_shader_module(dev, vert.content.view()),
                make_shader_module(dev, frag.content.view()),
            };

            // Stage information.
            psh::Buffer<VkPipelineShaderStageCreateInfo, 2> pipe_shader_stages_info{
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
            VkPipelineLayoutCreateInfo pipe_layout_info{
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                // TODO: remove hard coded descriptor set layout setup.
                .setLayoutCount         = 1,
                .pSetLayouts            = &descriptor_sets.layout,
                .pushConstantRangeCount = 0,
            };
            mina_vk_assert(vkCreatePipelineLayout(
                dev,
                &pipe_layout_info,
                nullptr,
                &graphics_pip.pipeline_layout));

            VkPipelineVertexInputStateCreateInfo pipe_vert_stage_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount   = 1,
                .pVertexBindingDescriptions      = &VERTEX_BINDING_DESCRIPTION,
                .vertexAttributeDescriptionCount = VERTEX_ATTRIBUTE_DESCRIPTION.size(),
                .pVertexAttributeDescriptions    = VERTEX_ATTRIBUTE_DESCRIPTION.buf,
            };

            constexpr VkPipelineInputAssemblyStateCreateInfo PIPE_INPUT_ASSEMBLY_INFO{
                .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                .primitiveRestartEnable = VK_FALSE,  // NOTE: this disables index buffers
            };

            VkViewport pipe_viewport{
                .x        = 0.0f,
                .y        = 0.0f,
                .width    = static_cast<f32>(surf_ext.width),
                .height   = static_cast<f32>(surf_ext.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            VkRect2D pipe_scissors{
                .offset = {.x = 0, .y = 0},
                .extent = surf_ext,
            };
            VkPipelineViewportStateCreateInfo pipe_viewport_info{
                .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports    = &pipe_viewport,
                .scissorCount  = 1,
                .pScissors     = &pipe_scissors,
            };

            constexpr VkPipelineRasterizationStateCreateInfo PIPE_RASTERIZATION_INFO{
                .sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode             = VK_POLYGON_MODE_FILL,
                .cullMode                = VK_CULL_MODE_NONE,
                .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .depthBiasEnable         = VK_FALSE,
                .lineWidth               = 1.0f,
            };
            constexpr VkPipelineMultisampleStateCreateInfo PIPE_SAMPLING_INFO{
                .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable   = VK_FALSE,
                .minSampleShading      = 1.0f,
                .alphaToCoverageEnable = VK_FALSE,
                .alphaToOneEnable      = VK_FALSE,
            };

            // Alpha-blending setup.
            constexpr VkPipelineColorBlendAttachmentState PIPE_BLEND_ATTACHMENT{
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
            VkPipelineColorBlendStateCreateInfo pipe_color_blending{
                .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable   = VK_FALSE,
                .attachmentCount = 1,
                .pAttachments    = &PIPE_BLEND_ATTACHMENT,
            };

            constexpr psh::Buffer<VkDynamicState, 2> PIPE_DYNAMICALLY_SET_STATES{
                VK_DYNAMIC_STATE_SCISSOR,
                VK_DYNAMIC_STATE_VIEWPORT,
            };
            VkPipelineDynamicStateCreateInfo pipe_dynamic_states_info{
                .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = PIPE_DYNAMICALLY_SET_STATES.size(),
                .pDynamicStates    = PIPE_DYNAMICALLY_SET_STATES.buf,
            };

            // Create the graphics pipeline.
            VkGraphicsPipelineCreateInfo pipe_info{
                .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount          = pipe_shader_stages_info.size(),
                .pStages             = pipe_shader_stages_info.buf,
                .pVertexInputState   = &pipe_vert_stage_info,
                .pInputAssemblyState = &PIPE_INPUT_ASSEMBLY_INFO,
                .pTessellationState  = nullptr,
                .pViewportState      = &pipe_viewport_info,
                .pRasterizationState = &PIPE_RASTERIZATION_INFO,
                .pMultisampleState   = &PIPE_SAMPLING_INFO,
                .pDepthStencilState  = nullptr,
                .pColorBlendState    = &pipe_color_blending,
                .pDynamicState       = &pipe_dynamic_states_info,
                .layout              = graphics_pip.pipeline_layout,
                .renderPass          = graphics_pip.render_pass.handle,
                .subpass             = 0,
            };
            mina_vk_assert(vkCreateGraphicsPipelines(
                dev,
                nullptr,
                1,
                &pipe_info,
                nullptr,
                &graphics_pip.handle));

            // Delete shaders.
            for (VkShaderModule shader : shaders) {
                vkDestroyShaderModule(dev, shader, nullptr);
            }
        }
    }

    void destroy_graphics_pipeline_context(VkDevice dev, Pipeline& graphics_pipe) noexcept {
        vkDestroyPipelineLayout(dev, graphics_pipe.pipeline_layout, nullptr);
        vkDestroyRenderPass(dev, graphics_pipe.render_pass.handle, nullptr);
        vkDestroyPipeline(dev, graphics_pipe.handle, nullptr);
    }
}  // namespace mina
