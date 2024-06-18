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
/// Description: Command buffer management.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/gfx/types.h>

namespace mina {
    // -----------------------------------------------------------------------------
    // - Command buffer lifetime management -
    // -----------------------------------------------------------------------------

    void create_command_buffers(
        VkDevice             dev,
        CommandManager&      cmds,
        QueueFamilies const& queues,
        psh::Arena*          arena,
        u32                  max_frames_in_flight) noexcept;

    void destroy_command_buffers(VkDevice dev, CommandManager& cmds) noexcept;

    // -----------------------------------------------------------------------------
    // - Data transfer commands -
    // -----------------------------------------------------------------------------

    TransferInfo transfer_whole_info(
        VkBuffer src,
        VkBuffer dst,
        usize    vertex_buf_size,
        usize    uniform_buf_size) noexcept;

    void record_transfer_commands(VkCommandBuffer transf_cmd, TransferInfo const& info) noexcept;

    void submit_transfer_commands(
        VkQueue         transf_queue,
        VkCommandBuffer transf_cmd,
        VkFence         transf_fence) noexcept;

    void wait_transfer_completion(VkDevice dev, VkFence transf_fence) noexcept;

    // -----------------------------------------------------------------------------
    // - Graphics rendering commands -
    // -----------------------------------------------------------------------------

    void record_graphics_commands(
        VkCommandBuffer        cmd_buf,
        QueueFamilies const&   queues,
        GraphicsCmdInfo const& info,
        RenderDataInfo const&  data_info) noexcept;

    void submit_graphics_commands(
        VkQueue         gfx_queue,
        VkCommandBuffer gfx_cmd,
        VkSemaphore     swc_sema,
        VkSemaphore     render_pass_sema,
        VkFence         frame_fence) noexcept;
}  // namespace mina
