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
    enum class ShaderCatalog {
        TriangleVertex,
        TriangleFragment,
        ShaderCatalogCount,
    };

    constexpr StrPtr shader_path(ShaderCatalog s) noexcept {
        StrPtr path;
        switch (s) {
            case ShaderCatalog::TriangleVertex:   path = "build/bin/triangle.vert.spv"; break;
            case ShaderCatalog::TriangleFragment: path = "build/bin/triangle.frag.spv"; break;
            default:                              psh_unreachable();
        }
        return path;
    }

    void create_graphics_pipeline(GraphicsContext& ctx) noexcept;

    void create_graphics_commands(GraphicsContext& ctx) noexcept;

    void submit_graphics_commands(GraphicsContext& ctx) noexcept;

    void destroy_graphics_pipeline(GraphicsContext& ctx) noexcept;
}  // namespace mina::gfx
