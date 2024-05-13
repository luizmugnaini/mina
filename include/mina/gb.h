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
/// Description: Game Boy emulator instance.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/cart.h>
#include <mina/cpu/dmg.h>
#include <mina/gfx/context.h>
#include <mina/memory_map.h>
#include <psh/arena.h>
#include <psh/memory_manager.h>

namespace mina {
    struct GameBoy {
        psh::MemoryManager   memory_manager{};
        psh::Arena           rom_arena{};
        psh::Arena           gfx_arena{};
        psh::Arena           frame_arena{};
        psh::Arena           work_arena{};
        gfx::GraphicsContext graphics_context{};
        GbCart               cart{};
        dmg::CPU             cpu{};

        GameBoy() noexcept;
        ~GameBoy() noexcept;
        void run(psh::StringView cart_path) noexcept;
    };
}  // namespace mina
