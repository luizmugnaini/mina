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
/// Description:  All exposed types used by the Vulkan graphics layer.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/gfx/vma.h>
#include <psh/array.h>
#include <psh/buffer.h>
#include <psh/dyn_array.h>
#include <psh/fat_ptr.h>
#include <psh/vec.h>
#include <vulkan/vulkan_core.h>

namespace mina {
    // -----------------------------------------------------------------------------
    // - Operation results -
    // -----------------------------------------------------------------------------

    enum struct FrameStatus {
        OK,
        NOT_READY,
        SWAP_CHAIN_OUT_OF_DATE,
        FATAL,
    };

    enum struct PresentStatus {
        OK,
        NOT_READY,
        SWAP_CHAIN_OUT_OF_DATE,
        FATAL,
        SURFACE_LOST,
        DEVICE_LOST,
    };

    // -----------------------------------------------------------------------------
    // - Synchronization objects -
    // -----------------------------------------------------------------------------

    struct FrameSemaphores {
        psh::Array<VkSemaphore> frame_semaphore = {};
    };

    struct FrameFences {
        psh::Array<VkFence> frame_fence = {};
    };

    struct SynchronizerManager {
        FrameFences     frame_in_flight      = {};
        FrameFences     finished_transfer    = {};
        FrameSemaphores image_available      = {};
        FrameSemaphores finished_render_pass = {};
    };

    // -----------------------------------------------------------------------------
    // - Data buffers -
    // -----------------------------------------------------------------------------

    // TODO: fill these buffer infos with meaningful numbers.

    // Host buffer information.
    [[maybe_unused]] constexpr usize HOST_BUFFER_SIZE             = 65536;
    [[maybe_unused]] constexpr usize VERTEX_STAGING_BUFFER_OFFSET = 0;
    [[maybe_unused]] constexpr usize VERTEX_STAGING_BUFFER_SIZE   = 1024;
    [[maybe_unused]] constexpr usize UNIFORM_STAGING_BUFFER_OFFSET =
        VERTEX_STAGING_BUFFER_SIZE + VERTEX_STAGING_BUFFER_OFFSET;
    [[maybe_unused]] constexpr usize UNIFORM_STAGING_BUFFER_SIZE = 1024;

    // Device buffer information.
    [[maybe_unused]] constexpr usize DEVICE_BUFFER_SIZE   = 65536;
    [[maybe_unused]] constexpr usize VERTEX_BUFFER_OFFSET = 0;
    [[maybe_unused]] constexpr usize VERTEX_BUFFER_SIZE   = 1024;
    [[maybe_unused]] constexpr usize UNIFORM_BUFFER_OFFSET =
        VERTEX_BUFFER_SIZE + VERTEX_BUFFER_OFFSET;
    [[maybe_unused]] constexpr usize UNIFORM_BUFFER_SIZE = 1024;

    /// Information regarding the staging of CPU data to the host staging buffer.
    struct StagingInfo {
        u8 const* src_ptr;
        usize     vertex_buf_size;
        usize     vertex_buf_src_offset;
        usize     vertex_buf_dst_offset;
        usize     uniform_buf_size;
        usize     uniform_buf_src_offset;
        usize     uniform_buf_dst_offset;
    };

    struct TransferInfo {
        VkBuffer src_buf_handle;
        VkBuffer dst_buf_handle;
        usize    src_buf_offset;
        usize    dst_buf_offset;
        usize    vertex_buf_size;
        usize    uniform_buf_size;
    };

    struct Buffer {
        VkBuffer      handle     = nullptr;
        VmaAllocation allocation = nullptr;
        usize         offset     = 0;
        usize         size       = 0;
    };

    struct HostBuffer {
        VkBuffer      handle     = nullptr;
        VmaAllocation allocation = nullptr;
        usize         size       = 0;
    };

    struct DeviceBuffer {
        VkBuffer      handle     = nullptr;
        VmaAllocation allocation = nullptr;
        usize         size       = 0;
    };

    // TODO: this should be POD, remove the methods and write get_host_buffer(), etc.
    struct BufferManager {
        HostBuffer   host   = {};
        DeviceBuffer device = {};

        Buffer host_buffer() const noexcept {
            return Buffer{
                .handle     = host.handle,
                .allocation = host.allocation,
                .offset     = 0,
                .size       = host.size,
            };
        }

        Buffer device_buffer() const noexcept {
            return Buffer{
                .handle     = device.handle,
                .allocation = device.allocation,
                .offset     = 0,
                .size       = device.size,
            };
        }

        Buffer vertex_staging_buffer() const noexcept {
            return Buffer{
                .handle     = host.handle,
                .allocation = host.allocation,
                .offset     = VERTEX_STAGING_BUFFER_OFFSET,
                .size       = VERTEX_STAGING_BUFFER_SIZE,
            };
        }

        Buffer uniform_staging_buffer() const noexcept {
            return Buffer{
                .handle     = host.handle,
                .allocation = host.allocation,
                .offset     = UNIFORM_STAGING_BUFFER_OFFSET,
                .size       = UNIFORM_STAGING_BUFFER_SIZE,
            };
        }

        Buffer vertex_buffer() const noexcept {
            return Buffer{
                .handle     = device.handle,
                .allocation = device.allocation,
                .offset     = VERTEX_BUFFER_OFFSET,
                .size       = VERTEX_BUFFER_SIZE,
            };
        }

        Buffer uniform_buffer() const noexcept {
            return Buffer{
                .handle     = device.handle,
                .allocation = device.allocation,
                .offset     = UNIFORM_BUFFER_OFFSET,
                .size       = UNIFORM_BUFFER_SIZE,
            };
        }
    };

    // -----------------------------------------------------------------------------
    // - Rendering pipeline related objects -
    // -----------------------------------------------------------------------------

    struct FrameResources {
        VkCommandBuffer transfer_cmd;
        VkCommandBuffer graphics_cmd;
        VkFence         frame_in_flight_fence;
        VkFence         transfer_ended_fence;
        VkSemaphore     image_available_semaphore;
        VkSemaphore     render_pass_ended_semaphore;
        VkImage         image     = nullptr;
        VkFramebuffer   frame_buf = nullptr;
    };

    struct RenderPass {
        VkRenderPass handle = nullptr;
    };

    struct FrameCommands {
        psh::Array<VkCommandBuffer> cmd = {};
    };

    struct CommandManager {
        VkCommandPool pool     = nullptr;
        FrameCommands graphics = {};
        FrameCommands transfer = {};
    };

    struct DescriptorSetManager {
        VkDescriptorSetLayout layout;
        VkDescriptorPool      pool;
        VkDescriptorSet       uniform_buf_descriptor_set;
    };

    struct RenderDataInfo {
        usize size;
        usize offset;
        u32   vertex_count;
        u32   instance_count;
        u32   first_vertex_index   = 0;
        u32   first_instance_index = 0;
    };

    struct GraphicsCmdInfo {
        VkPipeline       pipeline;
        VkPipelineLayout pipeline_layout;
        VkImage          image;
        RenderPass       render_pass;
        VkFramebuffer    frame_buf;
        VkExtent2D       surface_extent;
        VkBuffer         vertex_buf;
        u32              vertex_buf_binding;
        VkDescriptorSet  uniform_buf_descriptor_set;
        u32              uniform_buf_offset;
    };

    struct Pipeline {
        VkPipeline       handle          = nullptr;
        VkPipelineLayout pipeline_layout = nullptr;
        RenderPass       render_pass{};
    };

    struct PipelineManager {
        Pipeline graphics{};
    };

    struct SwapChainInfo {
        psh::Array<VkSurfaceFormatKHR> surface_formats      = {};
        psh::Array<VkPresentModeKHR>   presentation_modes   = {};
        VkSurfaceCapabilitiesKHR       surface_capabilities = {};
    };

    [[maybe_unused]] constexpr VkComponentMapping IMAGE_COMPONENT_MAPPING{
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    [[maybe_unused]] constexpr VkImageSubresourceRange IMAGE_SUBRESOURCE_RANGE{
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    struct SwapChain {
        VkSwapchainKHR            handle               = nullptr;
        psh::Array<VkImage>       images               = {};
        psh::Array<VkImageView>   image_views          = {};
        psh::Array<VkFramebuffer> frame_bufs           = {};
        VkExtent2D                extent               = {};
        VkSurfaceFormatKHR        surface_format       = {};
        u32                       max_frames_in_flight = 0;
        u32                       current_image_index  = 0;
        u32                       current_frame        = 0;
    };

    struct QueueFamilies {
        VkQueue graphics_queue = nullptr;
        VkQueue present_queue  = nullptr;
        u32     graphics_queue_index;
        u32     present_queue_index;

        inline psh::FatPtr<u32 const> indices() const {
            return psh::FatPtr{&this->graphics_queue_index, 2};
        }

        inline psh::DynArray<u32> unique_indices(psh::Arena* arena) const {
            psh::DynArray<u32> uidx{arena, 2};
            uidx.push(graphics_queue_index);
            if (psh_unlikely(present_queue_index != graphics_queue_index)) {
                uidx.push(present_queue_index);
            }
            return uidx;
        }
    };

    enum AttribFormat {
        SINGLE_U8  = VK_FORMAT_R8_UINT,
        SINGLE_I8  = VK_FORMAT_R8_SINT,
        VEC2_U8    = VK_FORMAT_R8G8_UINT,
        VEC2_I8    = VK_FORMAT_R8G8_SINT,
        VEC3_U8    = VK_FORMAT_R8G8B8_UINT,
        VEC3_I8    = VK_FORMAT_R8G8B8_SINT,
        VEC4_U8    = VK_FORMAT_R8G8B8A8_UINT,
        VEC4_I8    = VK_FORMAT_R8G8B8A8_SINT,
        SINGLE_U16 = VK_FORMAT_R16_UINT,
        SINGLE_I16 = VK_FORMAT_R16_SINT,
        SINGLE_F16 = VK_FORMAT_R16_SFLOAT,
        VEC2_U16   = VK_FORMAT_R16G16_UINT,
        VEC2_I16   = VK_FORMAT_R16G16_SINT,
        VEC2_F16   = VK_FORMAT_R16G16_SFLOAT,
        VEC3_U16   = VK_FORMAT_R16G16B16_UINT,
        VEC3_I16   = VK_FORMAT_R16G16B16_SINT,
        VEC3_F16   = VK_FORMAT_R16G16B16_SFLOAT,
        VEC4_U16   = VK_FORMAT_R16G16B16A16_UINT,
        VEC4_I16   = VK_FORMAT_R16G16B16A16_SINT,
        VEC4_F16   = VK_FORMAT_R16G16B16A16_SFLOAT,
        SINGLE_U32 = VK_FORMAT_R32_UINT,
        SINGLE_I32 = VK_FORMAT_R32_SINT,
        SINGLE_F32 = VK_FORMAT_R32_SFLOAT,
        VEC2_U32   = VK_FORMAT_R32G32_UINT,
        VEC2_I32   = VK_FORMAT_R32G32_SINT,
        VEC2_F32   = VK_FORMAT_R32G32_SFLOAT,
        VEC3_U32   = VK_FORMAT_R32G32B32_UINT,
        VEC3_I32   = VK_FORMAT_R32G32B32_SINT,
        VEC3_F32   = VK_FORMAT_R32G32B32_SFLOAT,
        VEC4_U32   = VK_FORMAT_R32G32B32A32_UINT,
        VEC4_I32   = VK_FORMAT_R32G32B32A32_SINT,
        VEC4_F32   = VK_FORMAT_R32G32B32A32_SFLOAT,
        SINGLE_U64 = VK_FORMAT_R64_UINT,
        SINGLE_I64 = VK_FORMAT_R64_SINT,
        SINGLE_F64 = VK_FORMAT_R64_SFLOAT,
        VEC2_U64   = VK_FORMAT_R64G64_UINT,
        VEC2_I64   = VK_FORMAT_R64G64_SINT,
        VEC2_F64   = VK_FORMAT_R64G64_SFLOAT,
        VEC3_U64   = VK_FORMAT_R64G64B64_UINT,
        VEC3_I64   = VK_FORMAT_R64G64B64_SINT,
        VEC3_F64   = VK_FORMAT_R64G64B64_SFLOAT,
        VEC4_U64   = VK_FORMAT_R64G64B64A64_UINT,
        VEC4_I64   = VK_FORMAT_R64G64B64A64_SINT,
        VEC4_F64   = VK_FORMAT_R64G64B64A64_SFLOAT,
    };
}  // namespace mina
