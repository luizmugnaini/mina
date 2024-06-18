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
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/gfx/types.h>

namespace mina {
    // -----------------------------------------------------------------------------
    // - Shader catalog -
    // -----------------------------------------------------------------------------

    enum struct ShaderCatalog {
        TRIANGLE_VERTEX,
        TRIANGLE_FRAGMENT,
        SHADER_COUNT,
    };

    constexpr strptr shader_path(ShaderCatalog s) noexcept {
        strptr path;
        switch (s) {
            case ShaderCatalog::TRIANGLE_VERTEX:   path = "build/bin/triangle.vert.spv"; break;
            case ShaderCatalog::TRIANGLE_FRAGMENT: path = "build/bin/triangle.frag.spv"; break;
            default:                               psh_unreachable();
        }
        return path;
    }
    // -----------------------------------------------------------------------------
    // - Descriptor set management -
    // -----------------------------------------------------------------------------

    void create_descriptor_sets(
        VkDevice              dev,
        DescriptorSetManager& descriptor_sets,
        Buffer const&         uniform_buf) noexcept;

    void destroy_descriptor_sets(VkDevice dev, DescriptorSetManager& descriptor_sets) noexcept;

    // -----------------------------------------------------------------------------
    // - Graphics pipeline context lifetime management -
    // -----------------------------------------------------------------------------

    void create_graphics_pipeline_context(
        VkDevice              dev,
        psh::Arena*           persistent_arena,
        Pipeline&             graphics_pip,
        DescriptorSetManager& descriptor_sets,
        VkFormat              surf_fmt,
        VkExtent2D            surf_ext) noexcept;

    void destroy_graphics_pipeline_context(VkDevice dev, Pipeline& graphics_pip) noexcept;
}  // namespace mina
