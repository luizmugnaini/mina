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

#include <mina/base.h>
#include <mina/cpu/dmg_reg.h>
#include <mina/memory_map.h>
#include <mina/utils/math.h>
#include <mina/utils/mem.h>

namespace mina::dmg {
    /// Conditional execution.
    ///
    /// Execute the instruction if the corresponding register flag is set.
    enum class Cond {
        Z,   ///< Z flag set.
        NZ,  ///< Z flag not set.

        C,   ///< C flag set.
        NC,  ///< C flag not set.
    };

    /// 3-bit unsigned integer constant.
    struct UConst3 {
        u8 val    : 3 = 0b000;
        u8 unused : 5;
    };

    enum class Opcode {
        Nop = 0x00,

        LD_bc_d16 = 0x01,
        LD_bc_a   = 0x02,

        INC_bc = 0x03,
        INC_b  = 0x04,

        DEC_b = 0x05,

        ADD_hl_bc  = 0x09,
        ADD_hl_de  = 0x19,
        ADD_hl_hl  = 0x29,
        ADD_hl_sp  = 0x39,
        ADD_a_b    = 0x80,
        ADD_a_c    = 0x81,
        ADD_a_d    = 0x82,
        ADD_a_e    = 0x83,
        ADD_a_h    = 0x84,
        ADD_a_l    = 0x85,
        ADD_a_hl   = 0x86,
        ADD_a_a    = 0x87,
        ADD_a_imm8 = 0xC6,
        ADD_sp_e8  = 0xE8,

        ADC_a_b    = 0x88,
        ADC_a_c    = 0x89,
        ADC_a_d    = 0x8A,
        ADC_a_e    = 0x8B,
        ADC_a_h    = 0x8C,
        ADC_a_l    = 0x8D,
        ADC_a_hl   = 0x8E,
        ADC_a_a    = 0x8F,
        ADC_a_imm8 = 0xCE,

        AND_b    = 0xA0,
        AND_c    = 0xA1,
        AND_d    = 0xA2,
        AND_e    = 0xA3,
        AND_h    = 0xA4,
        AND_l    = 0xA5,
        AND_a_hl = 0xA6,
        AND_a    = 0xA7,
        AND_d8   = 0xE6,
    };

    /// DMG, the original Game Boy CPU.
    ///
    /// The DMG is an SoC containing a Sharp SM83 CPU, which is based on the Zilog Z80 and
    /// Intel 8080.
    struct CPU {
        MemoryMap    mem{};
        CPURegisters reg{};

        /// Read the address in a cycle.
        u8 cycle_read(u16 addr) noexcept;

        /// Read immediate 8-bit value.
        u8 read_imm8() noexcept;

        /// Read immediate 16-bit value.
        u16 read_imm16() noexcept;

        /// Instructions: ADC A,r8; ADC A,[HL]; ADC A,n8
        void adc(u8 val) noexcept;

        /// Instructions: ADD HL,r16
        void add_hl_r16(u16 val) noexcept;

        /// Instructions: ADD A,r8; ADD A,[HL]; ADD A,n8
        void add(u8 val) noexcept;

        /// Instructions: ADD SP,e8
        void add_sp_e8(i8 offset) noexcept;

        /// Decode and execute an instruction.
        void dexec(u16 instr) noexcept;
    };
}  // namespace mina::dmg
