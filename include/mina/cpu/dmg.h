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
/// Description: Game Boy's "DMG" CPU.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/memory_map.h>
#include <psh/math.h>
#include <psh/mem_utils.h>
#include <psh/option.h>
#include <psh/types.h>

namespace mina::dmg {
    struct RegisterFile {
        u16 af = 0x0000;  ///< Hi: accumulator, Lo: flag.
        u16 bc = 0x0000;  ///< Hi: B, Lo: C.
        u16 de = 0x0000;  ///< Hi: D, Lo: E.
        u16 hl = 0x0000;  ///< Hi: H, Lo: L.
        u16 sp = 0x0000;  ///< Stack Pointer.
        u16 pc = 0x0000;  ///< Program counter.
    };

    /// DMG, the original Game Boy CPU.
    ///
    /// The DMG is an SoC containing a Sharp SM83 CPU, which is based on the Zilog Z80 and
    /// Intel 8080.
    struct CPU {
        RegisterFile regfile{};
        MemoryMap    mmap{};
        u16          bus_addr = 0x0000;
        u16          clock    = 0x0000;

        void run_cycle() noexcept;
    };
}  // namespace mina::dmg
