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
/// Description: mina/cpu/dmg.h implementation.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/cpu/dmg.h>

#include <psh/assert.h>

// TODO: Separate instructions from the cpu. This allows for a very fast execution of instructions
//       that only depend on a single index lookup to a buffer.

namespace mina::dmg {
    void CPU::add_hl_r16(u16 val) noexcept {
        u16 const hl = reg.read(Reg16::HL);

        u32 const res = hl + val;
        reg.hl        = static_cast<u16>(res);

        reg.af &= 0xFF00;  // Clear all flags.
        reg.set_flag_if(Flag::C, res > 0x0000FFFF);
        reg.set_flag_if(Flag::H, ((hl & 0x00FF) + (val & 0x00FF)) > 0x00FF);
    }

    void CPU::add(u8 val) noexcept {
        u8 const acc = reg.acc();

        u16 const res = acc + val;
        reg.af        = psh::as_high_u16(static_cast<u8>(res));  // All flags clear.

        reg.set_flag_if(Flag::C, res > 0x00FF);
        reg.set_flag_if(Flag::H, ((acc & 0x0F) + (val & 0x0F)) > 0x0F);
        reg.set_flag_if(Flag::Z, res == 0);
    }

    void CPU::add_sp_e8(i8 offset) noexcept {
        auto const res = static_cast<u16>(
            psh::lb_add(static_cast<i16>(reg.sp), static_cast<i16>(offset), static_cast<i16>(0)));
        reg.sp = res;

        reg.af &= 0xFF00;  // Clear all flags.
        reg.set_flag_if(Flag::C, res > 0x0000FFFF);
        reg.set_flag_if(Flag::H, ((reg.sp & 0x00FF) + offset) > 0x00FF);
    }

    void CPU::adc(u8 val) noexcept {
        u8 const acc   = reg.acc();
        u8 const carry = reg.flag(Flag::C);

        u16 const res = acc + val + carry;
        reg.af        = psh::as_high_u16(static_cast<u8>(res));  // All flags clear.

        reg.set_flag_if(Flag::C, res > 0x00FF);
        reg.set_flag_if(Flag::H, ((acc & 0x0F) + (val & 0x0F) + carry) > 0x0F);
        reg.set_flag_if(Flag::Z, res == 0);
    }

    void CPU::dexec(u16 instr) noexcept {
        auto opcode = static_cast<Opcode>(instr & 0x00FF);
        switch (opcode) {
            case (Opcode::Nop): {
                break;
            }

            // ADD
            case (Opcode::ADD_hl_bc): {
                add_hl_r16(reg.read(Reg16::BC));
                break;
            }
            case (Opcode::ADD_hl_de): {
                add_hl_r16(reg.read(Reg16::DE));
                break;
            }
            case (Opcode::ADD_hl_hl): {
                add_hl_r16(reg.read(Reg16::HL));
                break;
            }
            case (Opcode::ADD_hl_sp): {
                add_hl_r16(reg.read(Reg16::SP));
                break;
            }
            case (Opcode::ADD_a_b): {
                add(reg.read(Reg8::B));
                break;
            }
            case (Opcode::ADD_a_c): {
                add(reg.read(Reg8::C));
                break;
            }
            case (Opcode::ADD_a_d): {
                add(reg.read(Reg8::D));
                break;
            }
            case (Opcode::ADD_a_e): {
                add(reg.read(Reg8::E));
                break;
            }
            case (Opcode::ADD_a_h): {
                add(reg.read(Reg8::H));
                break;
            }
            case (Opcode::ADD_a_l): {
                add(reg.read(Reg8::L));
                break;
            }
            case (Opcode::ADD_a_hl): {
                add(cycle_read(reg.read(Reg16::HL)));
                break;
            }
            case (Opcode::ADD_a_a): {
                add(reg.read(Reg8::A));
                break;
            }
            case (Opcode::ADD_a_imm8): {
                add(read_imm8());
                break;
            }
            case (Opcode::ADD_sp_e8): {
                add_sp_e8(static_cast<i8>(cycle_read(reg.pc++)));
                break;
            }

            // ADC
            case (Opcode::ADC_a_b): {
                adc(reg.read(Reg8::B));
                break;
            }
            case (Opcode::ADC_a_c): {
                adc(reg.read(Reg8::C));
                break;
            }
            case (Opcode::ADC_a_d): {
                adc(reg.read(Reg8::D));
                break;
            }
            case (Opcode::ADC_a_e): {
                adc(reg.read(Reg8::E));
                break;
            }
            case (Opcode::ADC_a_h): {
                adc(reg.read(Reg8::H));
                break;
            }
            case (Opcode::ADC_a_l): {
                adc(reg.read(Reg8::L));
                break;
            }
            case (Opcode::ADC_a_a): {
                adc(reg.read(Reg8::A));
                break;
            }
            case (Opcode::ADC_a_hl): {
                adc(cycle_read(reg.read(Reg16::HL)));
                break;
            }
            case (Opcode::ADC_a_imm8): {
                adc(read_imm8());
                break;
            }
            default: psh_unreachable();
        }
    }
}  // namespace mina::dmg
