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
/// Description: GPU buffer management.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/gfx/types.h>
#include <vulkan/vulkan_core.h>

namespace mina {
    void create_buffers(VmaAllocator alloc, BufferManager& bufs, QueueFamilies& queues) noexcept;

    void destroy_buffers(VmaAllocator alloc, BufferManager& bufs) noexcept;

    void stage_host_data(
        VmaAllocator       alloc,
        Buffer const&      staging_buf,
        StagingInfo const& staging_info) noexcept;
}  // namespace mina
