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
/// Description: Game Boy's "DMG" CPU registers.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/memory_map.h>

#include <psh/mem_utils.h>
#include <psh/types.h>

namespace mina::dmg {
    /// 8-bit registers.
    enum class Reg8 : u8 {
        B = 0,
        C,
        D,
        E,
        H,
        L,
        A,
    };

    /// 16-bit registers.
    enum class Reg16 : u8 {
        BC = 0,
        DE,
        HL,
        SP,
    };

    enum class Flag : u8 {
        /// The carry flag. Set in the following occasions:
        ///     * 8-bit addition is higher than 0xFF.
        ///     * 16-bit addition is higher than 0xFFFF.
        ///     * Result of subtraction or comparison is negative.
        ///     * If a shift operation shifts out a 0b1 valued bit.
        C,
        H,  ///< Indicates carry for the high nibble.
        N,  ///< If the last operation was a subtraction.
        Z,  ///< If the last operation result was zero.
    };

    struct CPURegisters {
        u16 af;  ///< Hi: accumulator, Lo: flag.
        u16 bc;  ///< Hi: B, Lo: C.
        u16 de;  ///< Hi: D, Lo: E.
        u16 hl;  ///< Hi: H, Lo: L.
        u16 sp;  ///< Stack Pointer.
        u16 pc;  ///< Program counter.

        [[nodiscard]] constexpr u8 acc() const {
            return static_cast<u8>(af >> 8);
        }

        [[nodiscard]] constexpr u8 read(Reg8 r) const {
            u8 val;
            switch (r) {
                case (Reg8::A): val = static_cast<u8>(af >> 8); break;
                case (Reg8::B): val = static_cast<u8>(bc >> 8); break;
                case (Reg8::C): val = static_cast<u8>(bc & 0x00FF); break;
                case (Reg8::D): val = static_cast<u8>(de >> 8); break;
                case (Reg8::E): val = static_cast<u8>(de & 0x00FF); break;
                case (Reg8::H): val = static_cast<u8>(hl >> 8); break;
                case (Reg8::L): val = static_cast<u8>(hl & 0x00FF); break;
            }
            return val;
        }

        [[nodiscard]] constexpr u16 read(Reg16 r) const {
            u16 val;
            switch (r) {
                case (Reg16::BC): val = bc; break;
                case (Reg16::DE): val = de; break;
                case (Reg16::HL): val = hl; break;
                case (Reg16::SP): val = sp; break;
            }
            return val;
        }

        [[nodiscard]] constexpr u8 flag(Flag f) const {
            u8 flags = af & 0x00FF;

            u8 val;
            switch (f) {
                case (Flag::C): val = static_cast<u8>(flags << 4); break;
                case (Flag::H): val = static_cast<u8>(flags << 5); break;
                case (Flag::N): val = static_cast<u8>(flags << 6); break;
                case (Flag::Z): val = static_cast<u8>(flags << 7); break;
            }
            return val;
        }

        /// Set flag `f` if `cond` is satisfied, otherwise, clear the flag.
        constexpr void set_flag_if(Flag f, bool cond) {
            auto const icond = static_cast<i32>(cond);
            switch (f) {
                case (Flag::C): af ^= ((-icond ^ af) & psh::bit(4)); break;
                case (Flag::H): af ^= ((-icond ^ af) & psh::bit(5)); break;
                case (Flag::N): af ^= ((-icond ^ af) & psh::bit(6)); break;
                case (Flag::Z): af ^= ((-icond ^ af) & psh::bit(7)); break;
            }
        }

        constexpr void set_flag(Flag f) {
            switch (f) {
                case (Flag::C): af |= psh::bit(4); break;
                case (Flag::H): af |= psh::bit(5); break;
                case (Flag::N): af |= psh::bit(6); break;
                case (Flag::Z): af |= psh::bit(7); break;
            }
        }

        constexpr void clear_flag(Flag f) {
            switch (f) {
                case (Flag::C): af &= psh::clear_bit(4); break;
                case (Flag::H): af &= psh::clear_bit(5); break;
                case (Flag::N): af &= psh::clear_bit(6); break;
                case (Flag::Z): af &= psh::clear_bit(7); break;
            }
        }
    };
}  // namespace mina::dmg
