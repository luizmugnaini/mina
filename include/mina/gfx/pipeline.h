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
/// Description: Vulkan graphics pipeline management layer.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/gfx/context.h>
#include <psh/types.h>

namespace mina::gfx {
    enum struct ShaderCatalog {
        TRIANGLE_VERTEX,
        TRIANGLE_FRAGMENT,
        SHADER_CATALOG_COUNT,
    };

    constexpr strptr shader_path(ShaderCatalog sc) noexcept {
        strptr s;
        switch (sc) {
            case ShaderCatalog::TRIANGLE_VERTEX:   s = "build/bin/triangle.vert.spv"; break;
            case ShaderCatalog::TRIANGLE_FRAGMENT: s = "build/bin/triangle.frag.spv"; break;
            default:                              psh_unreachable();
        }
        return s;
    }

    void create_graphics_pipeline(GraphicsContext& ctx) noexcept;

    void create_graphics_commands(GraphicsContext& ctx) noexcept;

    void submit_graphics_commands(GraphicsContext& ctx) noexcept;

    void destroy_graphics_pipeline(GraphicsContext& ctx) noexcept;
}  // namespace mina::gfx
