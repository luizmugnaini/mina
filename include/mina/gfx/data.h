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
/// Description: Graphics data.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/gfx/types.h>
#include <psh/assert.h>
#include <psh/buffer.h>
#include <psh/fat_ptr.h>
#include <psh/memory_manager.h>
#include <psh/vec.h>

namespace mina {
    struct Vertex {
        psh::Vec3 position = {};
        psh::Vec3 color    = {};
    };

    struct VertexData {
        psh::FatPtr<Vertex> data;
    };

    constexpr VkVertexInputBindingDescription VERTEX_BINDING_DESCRIPTION{
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    constexpr psh::Buffer<VkVertexInputAttributeDescription, 2> VERTEX_ATTRIBUTE_DESCRIPTION{
        // Vertex position.
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding  = VERTEX_BINDING_DESCRIPTION.binding,
            .format   = static_cast<VkFormat>(AttribFormat::VEC3_F32),
            .offset   = 0,
        },
        // Vertex color.
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding  = VERTEX_BINDING_DESCRIPTION.binding,
            .format   = static_cast<VkFormat>(AttribFormat::VEC3_F32),
            .offset   = sizeof(Vertex::position),
        },
    };

    static constexpr usize VERTEX_BUF_SIZE  = psh_kibibytes(1);
    static constexpr usize UNIFORM_BUF_SIZE = 0;

    struct FrameMemory {
        psh::Arena arena;
        u32        vertex_count;
        u8*        vertex_buf;
        bool       vertex_buf_dirty;
    };

    FrameMemory create_frame_memory(psh::MemoryManager& memory_manager, f32 aspect) noexcept;

    VertexData vertex_data(FrameMemory const& frame_memory) noexcept;
    Vertex*    vertex_buffer_ptr(FrameMemory const& frame_memory) noexcept;

    StagingInfo    memory_staging_info(FrameMemory const& frame_memory) noexcept;
    RenderDataInfo render_data_info(FrameMemory const& frame_memory) noexcept;
}  // namespace mina
