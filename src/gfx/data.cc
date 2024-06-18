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
/// Description: Implementation of the graphics data manipulation procedures.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/gfx/data.h>

#include <psh/vec.h>

namespace mina {
    VertexData vertex_data(FrameMemory const& frame_memory) noexcept {
        return VertexData{
            psh::FatPtr<Vertex>{
                reinterpret_cast<Vertex*>(frame_memory.vertex_buf),
                VERTEX_BUF_SIZE / sizeof(Vertex)},
        };
    }

    Vertex* vertex_buffer_ptr(FrameMemory const& frame_memory) noexcept {
        return reinterpret_cast<Vertex*>(frame_memory.vertex_buf);
    }

    FrameMemory create_frame_memory(psh::MemoryManager& memory_manager, f32 aspect) noexcept {
        FrameMemory frame_memory;
        frame_memory.arena = memory_manager.make_arena(VERTEX_BUF_SIZE + UNIFORM_BUF_SIZE).demand();

        frame_memory.vertex_buf = frame_memory.arena.zero_alloc<u8>(VERTEX_BUF_SIZE);

        // Initialize the state of the vertices.
        {
            frame_memory.vertex_count = 3;
            Vertex* vertex_buf        = vertex_buffer_ptr(frame_memory);

            // Counter-clockwise face.
            vertex_buf[0].position = psh::Vec3{0.0f, -0.5f, 0.0f};
            vertex_buf[0].color    = psh::Vec3{1.0f, 0.0f, 0.0f};
            vertex_buf[1].position = psh::Vec3{0.5f, 0.5f, 0.0f};
            vertex_buf[1].color    = psh::Vec3{0.0f, 1.0f, 0.0f};
            vertex_buf[2].position = psh::Vec3{-0.5f, 0.5f, 0.0f};
            vertex_buf[2].color    = psh::Vec3{0.0f, 0.0f, 1.0f};
        }

        return frame_memory;
    }

    StagingInfo memory_staging_info(FrameMemory const& frame_memory) noexcept {
        return StagingInfo{
            .src_ptr                = frame_memory.arena.buf,
            .vertex_buf_size        = VERTEX_BUF_SIZE,
            .vertex_buf_src_offset  = 0,
            .vertex_buf_dst_offset  = 0,
            // NOTE(luiz): currently there is no uniform buffer.
            .uniform_buf_size       = 0,
            .uniform_buf_src_offset = 0,
            .uniform_buf_dst_offset = 0,
        };
    }

    RenderDataInfo render_data_info(FrameMemory const& frame_memory) noexcept {
        return RenderDataInfo{
            .size                 = VERTEX_BUF_SIZE,
            .offset               = 0,
            .vertex_count         = frame_memory.vertex_count,
            .instance_count       = 1,
            .first_vertex_index   = 0,
            .first_instance_index = 0,
        };
    }
}  // namespace mina
