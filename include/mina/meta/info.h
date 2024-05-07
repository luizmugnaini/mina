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
/// Description: Metadata about the Mina emulator.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <psh/types.h>

#define MINA_VULKAN_API_VERSION VK_API_VERSION_1_3
#define MINA_VMA_VULKAN_VERSION 1003000

namespace mina {
    [[maybe_unused]] constexpr StrPtr EMU_NAME      = "Mina Game Boy Emulator";
    [[maybe_unused]] constexpr StrPtr ENGINE_NAME   = "Mina";
    [[maybe_unused]] constexpr u32    MAJOR_VERSION = 0;
    [[maybe_unused]] constexpr u32    MINOR_VERSION = 0;
    [[maybe_unused]] constexpr u32    PATCH_VERSION = 1;
}  // namespace mina
