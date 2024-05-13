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
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/gfx/vma.h>
#include <psh/array.h>
#include <psh/buffer.h>
#include <psh/dyn_array.h>
#include <psh/fat_ptr.h>
#include <vulkan/vulkan_core.h>

namespace mina::gfx {
    struct Synchronizers {
        psh::Array<VkFence>     frame_in_flight{};
        VkFence                 buffer_transfer{};  // TODO: implement fencing for data transfer.
        psh::Array<VkSemaphore> swap_chain{};
        psh::Array<VkSemaphore> render_pass{};
    };

    struct MakeGPUBuffersInfo {
        usize staging_size = 65536;
        usize data_size    = 65536;
    };

    struct GPUBuffer {
        VkBuffer      handle;
        VmaAllocation allocation;
        usize         offset;
        usize         size;
        usize         vertex_count;
        u32           binding;
    };

    struct GPUBuffers {
        GPUBuffer staging;
        GPUBuffer data;
    };

    struct Commands {
        VkCommandPool               pool = nullptr;
        psh::Array<VkCommandBuffer> gfx_buf{};
        VkCommandBuffer             staging_buf{};
    };

    struct RenderPass {
        VkRenderPass handle = nullptr;
    };

    struct Pipeline {
        VkPipeline       handle = nullptr;
        VkPipelineLayout layout = nullptr;
        RenderPass       pass{};
    };

    struct Pipelines {
        Pipeline graphics{};
    };

    struct SwapChainInfo {
        psh::Array<VkSurfaceFormatKHR> surf_fmt{};
        psh::Array<VkPresentModeKHR>   pres_modes{};
        VkSurfaceCapabilitiesKHR       surf_capa{};
    };

    struct SwapChain {
        VkSwapchainKHR            handle = nullptr;
        psh::Array<VkImage>       img{};
        psh::Array<VkImageView>   img_view{};
        psh::Array<VkFramebuffer> frame_buf{};
        VkExtent2D                extent{};
        VkSurfaceFormatKHR        surf_fmt{};
        u32                       max_frames_in_flight = 0;
        u32                       current_img_idx      = 0;
        u32                       current_frame        = 0;
    };

    struct QueueFamilies {
        VkQueue graphics = nullptr;
        VkQueue transfer = nullptr;
        VkQueue present  = nullptr;
        u32     graphics_idx;
        u32     transfer_idx;
        u32     present_idx;

        inline psh::FatPtr<u32 const> indices() const {
            return psh::FatPtr{&this->graphics_idx, 3};
        }

        inline psh::DynArray<u32> unique_indices(psh::Arena* arena) const {
            psh::DynArray<u32> uidx{arena, 3};
            uidx.push(graphics_idx);
            if (transfer_idx != graphics_idx) {
                uidx.push(transfer_idx);
            }
            if (psh_unlikely(present_idx != graphics_idx)) {
                if (present_idx != transfer_idx) {
                    uidx.push(present_idx);
                }
            }
            return uidx;
        }
    };

    enum AttribFormat {
        Single_u8  = VK_FORMAT_R8_UINT,
        Single_i8  = VK_FORMAT_R8_SINT,
        Vec2_u8    = VK_FORMAT_R8G8_UINT,
        Vec2_i8    = VK_FORMAT_R8G8_SINT,
        Vec3_u8    = VK_FORMAT_R8G8B8_UINT,
        Vec3_i8    = VK_FORMAT_R8G8B8_SINT,
        Vec4_u8    = VK_FORMAT_R8G8B8A8_UINT,
        Vec4_i8    = VK_FORMAT_R8G8B8A8_SINT,
        Single_u16 = VK_FORMAT_R16_UINT,
        Single_i16 = VK_FORMAT_R16_SINT,
        Single_f16 = VK_FORMAT_R16_SFLOAT,
        Vec2_u16   = VK_FORMAT_R16G16_UINT,
        Vec2_i16   = VK_FORMAT_R16G16_SINT,
        Vec2_f16   = VK_FORMAT_R16G16_SFLOAT,
        Vec3_u16   = VK_FORMAT_R16G16B16_UINT,
        Vec3_i16   = VK_FORMAT_R16G16B16_SINT,
        Vec3_f16   = VK_FORMAT_R16G16B16_SFLOAT,
        Vec4_u16   = VK_FORMAT_R16G16B16A16_UINT,
        Vec4_i16   = VK_FORMAT_R16G16B16A16_SINT,
        Vec4_f16   = VK_FORMAT_R16G16B16A16_SFLOAT,
        Single_u32 = VK_FORMAT_R32_UINT,
        Single_i32 = VK_FORMAT_R32_SINT,
        Single_f32 = VK_FORMAT_R32_SFLOAT,
        Vec2_u32   = VK_FORMAT_R32G32_UINT,
        Vec2_i32   = VK_FORMAT_R32G32_SINT,
        Vec2_f32   = VK_FORMAT_R32G32_SFLOAT,
        Vec3_u32   = VK_FORMAT_R32G32B32_UINT,
        Vec3_i32   = VK_FORMAT_R32G32B32_SINT,
        Vec3_f32   = VK_FORMAT_R32G32B32_SFLOAT,
        Vec4_u32   = VK_FORMAT_R32G32B32A32_UINT,
        Vec4_i32   = VK_FORMAT_R32G32B32A32_SINT,
        Vec4_f32   = VK_FORMAT_R32G32B32A32_SFLOAT,
        Single_u64 = VK_FORMAT_R64_UINT,
        Single_i64 = VK_FORMAT_R64_SINT,
        Single_f64 = VK_FORMAT_R64_SFLOAT,
        Vec2_u64   = VK_FORMAT_R64G64_UINT,
        Vec2_i64   = VK_FORMAT_R64G64_SINT,
        Vec2_f64   = VK_FORMAT_R64G64_SFLOAT,
        Vec3_u64   = VK_FORMAT_R64G64B64_UINT,
        Vec3_i64   = VK_FORMAT_R64G64B64_SINT,
        Vec3_f64   = VK_FORMAT_R64G64B64_SFLOAT,
        Vec4_u64   = VK_FORMAT_R64G64B64A64_UINT,
        Vec4_i64   = VK_FORMAT_R64G64B64A64_SINT,
        Vec4_f64   = VK_FORMAT_R64G64B64A64_SFLOAT,
    };
}  // namespace mina::gfx
