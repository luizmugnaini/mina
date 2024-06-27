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
/// Description: Original DMG Game Boy's Sharp SM83 CPU.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/memory_map.h>
#include <psh/math.h>
#include <psh/mem_utils.h>
#include <psh/option.h>
#include <psh/types.h>

namespace mina {
    /// CPU register file.
    ///
    /// Note: In order to avoid dealing with architecture endianness, we are going to separate each
    ///       16-bit register into their 8-bit components and work always with little-endiannes. The
    ///       reasoning for choosing little-endian is because the Game Boy itself and most modern
    ///       architectures are little-endian.
    ///
    ///       The only register that don't follow this rule is the program counter, since they are
    ///       never accessed by their byte components. One would think that the stack pointer should
    ///       be made into a word, however in order to have a universal access method for every
    ///       16-bit register it is best that we separate the stack pointer into its high and low
    ///       bytes.
    struct RegisterFile {
        /// AF 16-bit register.
        u8  f  = 0x00;
        u8  a  = 0x00;
        /// BC 16-bit register.
        u8  c  = 0x00;
        u8  b  = 0x00;
        /// DE 16-bit register.
        u8  e  = 0x00;
        u8  d  = 0x00;
        /// HL 16-bit register.
        u8  l  = 0x00;
        u8  h  = 0x00;
        /// Stack pointer.
        u16 sp = 0x0000;
        /// Program counter.
        u16 pc = 0x0000;
    };

    /// Sharp SM83, the original Game Boy CPU.
    ///
    /// This is a system on a chip (SoC) based on the Zilog Z80 and Intel 8080.
    struct SM83 {
        RegisterFile regfile  = {};
        MemoryMap    mmap     = {};
        u16          bus_addr = 0x0000;
        u16          clock    = 0x0000;
    };

    void sm83_run_cycle(SM83& cpu) noexcept;
}  // namespace mina
