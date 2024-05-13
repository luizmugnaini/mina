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
#include <psh/intrinsics.h>

// TODO: detach behaviour from the CPU struct so that we can pass references instead of pointers.

namespace mina::dmg {
    // Implementation of the instructions
    namespace {
        void add_hl_r16(CPU* cpu, Reg16 reg) noexcept {
            u16 const val = cpu->reg.read(reg);
            u16 const hl  = cpu->reg.read(Reg16::HL);

            u32 const res = hl + val;
            cpu->reg.hl   = static_cast<u16>(res);

            cpu->reg.af &= 0xFF00;  // Clear all flags.
            cpu->reg.set_or_clear_flag_if(Flag::C, res > 0x0000FFFF);
            cpu->reg.set_or_clear_flag_if(Flag::H, ((hl & 0x00FF) + (val & 0x00FF)) > 0x00FF);
        }

        void add(CPU* cpu, u8 val) noexcept {
            u8 const acc = cpu->reg.acc();

            u16 const res = acc + val;
            cpu->reg.af   = psh::as_high_u16(static_cast<u8>(res));  // All flags clear.

            cpu->reg.set_or_clear_flag_if(Flag::C, res > 0x00FF);
            cpu->reg.set_or_clear_flag_if(Flag::H, ((acc & 0x0F) + (val & 0x0F)) > 0x0F);
            cpu->reg.set_or_clear_flag_if(Flag::Z, res == 0);
        }

        void add_sp_e8(CPU* cpu, i8 offset) noexcept {
            auto const res = static_cast<u16>(psh_lb_add(
                static_cast<i16>(cpu->reg.sp),
                static_cast<i16>(offset),
                static_cast<i16>(0)));
            cpu->reg.sp    = res;

            cpu->reg.af &= 0xFF00;  // Clear all flags.
            cpu->reg.set_or_clear_flag_if(Flag::C, res > 0x0000FFFF);
            cpu->reg.set_or_clear_flag_if(Flag::H, ((cpu->reg.sp & 0x00FF) + offset) > 0x00FF);
        }

        void adc(CPU* cpu, u8 val) noexcept {
            u8 const acc   = cpu->reg.acc();
            u8 const carry = cpu->reg.flag(Flag::C);

            u16 const res = acc + val + carry;
            cpu->reg.af   = psh::as_high_u16(static_cast<u8>(res));  // All flags clear.

            cpu->reg.set_or_clear_flag_if(Flag::C, res > 0x00FF);
            cpu->reg.set_or_clear_flag_if(Flag::H, ((acc & 0x0F) + (val & 0x0F) + carry) > 0x0F);
            cpu->reg.set_or_clear_flag_if(Flag::Z, res == 0);
        }

        void jp_u16(CPU* cpu) noexcept {
            cpu->reg.pc = cpu->bus_read_imm16();
        }
    }  // namespace

    u8 CPURegisters::acc() const {
        return static_cast<u8>(af >> 8);
    }

    u8 CPURegisters::read(Reg8 r) const {
        u8 val;
        switch (r) {
            case Reg8::A: val = static_cast<u8>(af >> 8); break;
            case Reg8::B: val = static_cast<u8>(bc >> 8); break;
            case Reg8::C: val = static_cast<u8>(bc & 0x00FF); break;
            case Reg8::D: val = static_cast<u8>(de >> 8); break;
            case Reg8::E: val = static_cast<u8>(de & 0x00FF); break;
            case Reg8::H: val = static_cast<u8>(hl >> 8); break;
            case Reg8::L: val = static_cast<u8>(hl & 0x00FF); break;
        }
        return val;
    }

    u16 CPURegisters::read(Reg16 r) const {
        u16 val;
        switch (r) {
            case Reg16::BC: val = bc; break;
            case Reg16::DE: val = de; break;
            case Reg16::HL: val = hl; break;
            case Reg16::SP: val = sp; break;
        }
        return val;
    }

    u8 CPURegisters::flag(Flag f) const {
        u8 flags = af & 0x00FF;

        u8 val;
        switch (f) {
            case Flag::C: val = static_cast<u8>(flags << 4); break;
            case Flag::H: val = static_cast<u8>(flags << 5); break;
            case Flag::N: val = static_cast<u8>(flags << 6); break;
            case Flag::Z: val = static_cast<u8>(flags << 7); break;
        }
        return val;
    }

    void CPURegisters::set_flag(Flag f) {
        switch (f) {
            case Flag::C: af |= psh::bit(4);
            case Flag::H: af |= psh::bit(5);
            case Flag::N: af |= psh::bit(6);
            case Flag::Z: af |= psh::bit(7);
        }
    }

    void CPURegisters::clear_flag(Flag f) {
        switch (f) {
            case Flag::C: af &= psh::clear_bit(4);
            case Flag::H: af &= psh::clear_bit(5);
            case Flag::N: af &= psh::clear_bit(6);
            case Flag::Z: af &= psh::clear_bit(7);
        }
    }

    void CPURegisters::set_or_clear_flag_if(Flag f, bool cond) {
        auto const icond = static_cast<i32>(cond);
        switch (f) {
            case Flag::C: af ^= ((-icond ^ af) & psh::bit(4)); break;
            case Flag::H: af ^= ((-icond ^ af) & psh::bit(5)); break;
            case Flag::N: af ^= ((-icond ^ af) & psh::bit(6)); break;
            case Flag::Z: af ^= ((-icond ^ af) & psh::bit(7)); break;
        }
    }

    u8 CPU::bus_read_byte(u16 addr) noexcept {
        u8 const* memory = mmap.memstart();
        bus_addr         = addr;
        return memory[bus_addr];
    }

    u8 CPU::bus_read_imm8() noexcept {
        u8 const* memory = mmap.memstart();
        u8 const  bt     = memory[reg.pc++];
        bus_addr         = reg.pc;
        return bt;
    }

    u16 CPU::bus_read_imm16() noexcept {
        u8 const* memory = mmap.memstart();
        u8 const  lo     = memory[reg.pc++];
        u8 const  hi     = memory[reg.pc++];
        bus_addr         = reg.pc;
        return psh::make_u16(hi, lo);
    }

    void CPU::dexec(u8 data) noexcept {
        auto opcode = static_cast<Opcode>(data);
        switch (opcode) {
            case Opcode::Nop: {
                break;
            }
            case Opcode::LD_bc_u16: {
                break;
            }
            case Opcode::LD_bc_ptr_a: {
                break;
            }
            case Opcode::INC_bc: {
                break;
            }
            case Opcode::INC_b: {
                break;
            }
            case Opcode::DEC_b: {
                break;
            }
            case Opcode::LD_b_u8: {
                break;
            }
            case Opcode::RLCA: {
                break;
            }
            case Opcode::LD_u16_ptr_sp: {
                break;
            }
            case Opcode::ADD_hl_bc: {
                add_hl_r16(this, Reg16::BC);
                break;
            }
            case Opcode::LD_a_bc_ptr: {
                break;
            }
            case Opcode::DEC_bc: {
                break;
            }
            case Opcode::INC_c: {
                break;
            }
            case Opcode::DEC_c: {
                break;
            }
            case Opcode::LD_c_u8: {
                break;
            }
            case Opcode::RRCA: {
                break;
            }
            case Opcode::STOP: {
                break;
            }
            case Opcode::LD_de_u16: {
                break;
            }
            case Opcode::LD_de_ptr_a: {
                break;
            }
            case Opcode::INC_de: {
                break;
            }
            case Opcode::INC_d: {
                break;
            }
            case Opcode::DEC_d: {
                break;
            }
            case Opcode::LD_d_u8: {
                break;
            }
            case Opcode::RLA: {
                break;
            }
            case Opcode::JR_i8: {
                break;
            }
            case Opcode::ADD_hl_de: {
                add_hl_r16(this, Reg16::DE);
                break;
            }
            case Opcode::LD_a_de_ptr: {
                break;
            }
            case Opcode::DEC_de: {
                break;
            }
            case Opcode::INC_e: {
                break;
            }
            case Opcode::DEC_e: {
                break;
            }
            case Opcode::LD_e_u8: {
                break;
            }
            case Opcode::RRA: {
                break;
            }
            case Opcode::JR_nz_i8: {
                break;
            }
            case Opcode::LD_hl_u16: {
                break;
            }
            case Opcode::LD_high_hl_ptr_a: {
                break;
            }
            case Opcode::INC_hl: {
                break;
            }
            case Opcode::INC_h: {
                break;
            }
            case Opcode::DEC_h: {
                break;
            }
            case Opcode::LD_h_u8: {
                break;
            }
            case Opcode::DAA: {
                break;
            }
            case Opcode::JR_z_i8: {
                break;
            }
            case Opcode::ADD_hl_hl: {
                add_hl_r16(this, Reg16::HL);
                break;
            }
            case Opcode::LD_a_high_hl_ptr: {
                break;
            }
            case Opcode::DEC_hl: {
                break;
            }
            case Opcode::INC_l: {
                break;
            }
            case Opcode::DEC_l: {
                break;
            }
            case Opcode::LD_l_u8: {
                break;
            }
            case Opcode::CPL: {
                break;
            }
            case Opcode::JR_nc_i8: {
                break;
            }
            case Opcode::LD_sp_u16: {
                break;
            }
            case Opcode::LD_low_hl_ptr_a: {
                break;
            }
            case Opcode::INC_sp: {
                break;
            }
            case Opcode::INC_hl_ptr: {
                break;
            }
            case Opcode::DEC_hl_ptr: {
                break;
            }
            case Opcode::LD_hl_ptr_u8: {
                break;
            }
            case Opcode::SCF: {
                break;
            }
            case Opcode::JR_c_i8: {
                break;
            }
            case Opcode::ADD_hl_sp: {
                add_hl_r16(this, Reg16::SP);
                break;
            }
            case Opcode::LD_a_low_hl_ptr: {
                break;
            }
            case Opcode::DEC_sp: {
                break;
            }
            case Opcode::INC_a: {
                break;
            }
            case Opcode::DEC_a: {
                break;
            }
            case Opcode::LD_a_u8: {
                break;
            }
            case Opcode::CCF: {
                break;
            }
            case Opcode::LD_b_b: {
                break;
            }
            case Opcode::LD_b_c: {
                break;
            }
            case Opcode::LD_b_d: {
                break;
            }
            case Opcode::LD_b_e: {
                break;
            }
            case Opcode::LD_b_h: {
                break;
            }
            case Opcode::LD_b_l: {
                break;
            }
            case Opcode::LD_b_hl_ptr: {
                break;
            }
            case Opcode::LD_b_a: {
                break;
            }
            case Opcode::LD_c_b: {
                break;
            }
            case Opcode::LD_c_c: {
                break;
            }
            case Opcode::LD_c_d: {
                break;
            }
            case Opcode::LD_c_e: {
                break;
            }
            case Opcode::LD_c_h: {
                break;
            }
            case Opcode::LD_c_l: {
                break;
            }
            case Opcode::LD_c_hl_ptr: {
                break;
            }
            case Opcode::LD_c_a: {
                break;
            }
            case Opcode::LD_d_b: {
                break;
            }
            case Opcode::LD_d_c: {
                break;
            }
            case Opcode::LD_d_d: {
                break;
            }
            case Opcode::LD_d_e: {
                break;
            }
            case Opcode::LD_d_h: {
                break;
            }
            case Opcode::LD_d_l: {
                break;
            }
            case Opcode::LD_d_hl_ptr: {
                break;
            }
            case Opcode::LD_d_a: {
                break;
            }
            case Opcode::LD_e_b: {
                break;
            }
            case Opcode::LD_e_c: {
                break;
            }
            case Opcode::LD_e_d: {
                break;
            }
            case Opcode::LD_e_e: {
                break;
            }
            case Opcode::LD_e_h: {
                break;
            }
            case Opcode::LD_e_l: {
                break;
            }
            case Opcode::LD_e_hl_ptr: {
                break;
            }
            case Opcode::LD_e_a: {
                break;
            }
            case Opcode::LD_h_b: {
                break;
            }
            case Opcode::LD_h_c: {
                break;
            }
            case Opcode::LD_h_d: {
                break;
            }
            case Opcode::LD_h_e: {
                break;
            }
            case Opcode::LD_h_h: {
                break;
            }
            case Opcode::LD_h_l: {
                break;
            }
            case Opcode::LD_h_hl_ptr: {
                break;
            }
            case Opcode::LD_h_a: {
                break;
            }
            case Opcode::LD_l_b: {
                break;
            }
            case Opcode::LD_l_c: {
                break;
            }
            case Opcode::LD_l_d: {
                break;
            }
            case Opcode::LD_l_e: {
                break;
            }
            case Opcode::LD_l_h: {
                break;
            }
            case Opcode::LD_l_l: {
                break;
            }
            case Opcode::LD_l_hl_ptr: {
                break;
            }
            case Opcode::LD_l_a: {
                break;
            }
            case Opcode::LD_hl_ptr_b: {
                break;
            }
            case Opcode::LD_hl_ptr_c: {
                break;
            }
            case Opcode::LD_hl_ptr_d: {
                break;
            }
            case Opcode::LD_hl_ptr_e: {
                break;
            }
            case Opcode::LD_hl_ptr_h: {
                break;
            }
            case Opcode::LD_hl_ptr_l: {
                break;
            }
            case Opcode::HALT: {
                break;
            }
            case Opcode::LD_hl_ptr_a: {
                break;
            }
            case Opcode::LD_a_b: {
                break;
            }
            case Opcode::LD_a_c: {
                break;
            }
            case Opcode::LD_a_d: {
                break;
            }
            case Opcode::LD_a_e: {
                break;
            }
            case Opcode::LD_a_h: {
                break;
            }
            case Opcode::LD_a_l: {
                break;
            }
            case Opcode::LD_a_hl_ptr: {
                break;
            }
            case Opcode::LD_a_a: {
                break;
            }
            case Opcode::ADD_a_b: {
                add(this, reg.read(Reg8::B));
                break;
            }
            case Opcode::ADD_a_c: {
                add(this, reg.read(Reg8::C));
                break;
            }
            case Opcode::ADD_a_d: {
                add(this, reg.read(Reg8::D));
                break;
            }
            case Opcode::ADD_a_e: {
                add(this, reg.read(Reg8::E));
                break;
            }
            case Opcode::ADD_a_h: {
                add(this, reg.read(Reg8::H));
                break;
            }
            case Opcode::ADD_a_l: {
                add(this, reg.read(Reg8::L));
                break;
            }
            case Opcode::ADD_a_hl_ptr: {
                add(this, static_cast<u8>(this->bus_read_byte(reg.read(Reg16::HL))));
                break;
            }
            case Opcode::ADD_a_a: {
                add(this, reg.read(Reg8::A));
                break;
            }
            case Opcode::ADC_a_b: {
                adc(this, reg.read(Reg8::B));
                break;
            }
            case Opcode::ADC_a_c: {
                adc(this, reg.read(Reg8::C));
                break;
            }
            case Opcode::ADC_a_d: {
                adc(this, reg.read(Reg8::D));
                break;
            }
            case Opcode::ADC_a_e: {
                adc(this, reg.read(Reg8::E));
                break;
            }
            case Opcode::ADC_a_h: {
                adc(this, reg.read(Reg8::H));
                break;
            }
            case Opcode::ADC_a_l: {
                adc(this, reg.read(Reg8::L));
                break;
            }
            case Opcode::ADC_a_hl: {
                adc(this, static_cast<u8>(this->bus_read_byte(reg.read(Reg16::HL))));
                break;
            }
            case Opcode::ADC_a_a: {
                adc(this, reg.read(Reg8::A));
                break;
            }
            case Opcode::SUB_a_b: {
                break;
            }
            case Opcode::SUB_a_c: {
                break;
            }
            case Opcode::SUB_a_d: {
                break;
            }
            case Opcode::SUB_a_e: {
                break;
            }
            case Opcode::SUB_a_h: {
                break;
            }
            case Opcode::SUB_a_l: {
                break;
            }
            case Opcode::SUB_a_hl_ptr: {
                break;
            }
            case Opcode::SUB_a_a: {
                break;
            }
            case Opcode::SBC_a_b: {
                break;
            }
            case Opcode::SBC_a_c: {
                break;
            }
            case Opcode::SBC_a_d: {
                break;
            }
            case Opcode::SBC_a_e: {
                break;
            }
            case Opcode::SBC_a_h: {
                break;
            }
            case Opcode::SBC_a_l: {
                break;
            }
            case Opcode::SBC_a_hl_ptr: {
                break;
            }
            case Opcode::SBC_a_a: {
                break;
            }
            case Opcode::AND_a_b: {
                break;
            }
            case Opcode::AND_a_c: {
                break;
            }
            case Opcode::AND_a_d: {
                break;
            }
            case Opcode::AND_a_e: {
                break;
            }
            case Opcode::AND_a_h: {
                break;
            }
            case Opcode::AND_a_l: {
                break;
            }
            case Opcode::AND_a_hl_ptr: {
                break;
            }
            case Opcode::AND_a_a: {
                break;
            }
            case Opcode::XOR_a_b: {
                break;
            }
            case Opcode::XOR_a_c: {
                break;
            }
            case Opcode::XOR_a_d: {
                break;
            }
            case Opcode::XOR_a_e: {
                break;
            }
            case Opcode::XOR_a_h: {
                break;
            }
            case Opcode::XOR_a_l: {
                break;
            }
            case Opcode::XOR_a_hl_ptr: {
                break;
            }
            case Opcode::XOR_a_a: {
                break;
            }
            case Opcode::OR_a_b: {
                break;
            }
            case Opcode::OR_a_c: {
                break;
            }
            case Opcode::OR_a_d: {
                break;
            }
            case Opcode::OR_a_e: {
                break;
            }
            case Opcode::OR_a_h: {
                break;
            }
            case Opcode::OR_a_l: {
                break;
            }
            case Opcode::OR_a_hl_ptr: {
                break;
            }
            case Opcode::OR_a_a: {
                break;
            }
            case Opcode::CP_a_b: {
                break;
            }
            case Opcode::CP_a_c: {
                break;
            }
            case Opcode::CP_a_d: {
                break;
            }
            case Opcode::CP_a_e: {
                break;
            }
            case Opcode::CP_a_h: {
                break;
            }
            case Opcode::CP_a_l: {
                break;
            }
            case Opcode::CP_a_hl_ptr: {
                break;
            }
            case Opcode::CP_a_a: {
                break;
            }
            case Opcode::RET_nz: {
                break;
            }
            case Opcode::POP_bc: {
                break;
            }
            case Opcode::JP_zn_u16: {
                break;
            }
            case Opcode::JP_u16: {
                jp_u16(this);
                break;
            }
            case Opcode::CALL_nz_u16: {
                break;
            }
            case Opcode::PUSH_bc: {
                break;
            }
            case Opcode::ADD_a_u8: {
                add(this, this->bus_read_imm8());
                break;
            }
            case Opcode::RST_0x00: {
                break;
            }
            case Opcode::RET_z: {
                break;
            }
            case Opcode::RET: {
                break;
            }
            case Opcode::JP_z_u16: {
                break;
            }
            case Opcode::PREFIX_0xCB: {
                psh_todo();
                break;
            }
            case Opcode::CALL_z_u16: {
                break;
            }
            case Opcode::CALL_u16: {
                break;
            }
            case Opcode::ADC_a_u8: {
                adc(this, this->bus_read_imm8());
                break;
            }
            case Opcode::RST_0x08: {
                break;
            }
            case Opcode::RET_nc: {
                break;
            }
            case Opcode::POP_de: {
                break;
            }
            case Opcode::JP_zc_u16: {
                break;
            }
            case Opcode::CALL_nc_u16: {
                break;
            }
            case Opcode::PUSH_de: {
                break;
            }
            case Opcode::SUB_a_u8: {
                break;
            }
            case Opcode::RST_0x10: {
                break;
            }
            case Opcode::RET_c: {
                break;
            }
            case Opcode::RETI: {
                break;
            }
            case Opcode::JP_c_u16: {
                break;
            }
            case Opcode::CALL_c_u16: {
                break;
            }
            case Opcode::SBC_a_u8: {
                break;
            }
            case Opcode::RST_0x18: {
                break;
            }
            case Opcode::LD_0xFF00_plus_u8_a: {
                break;
            }
            case Opcode::POP_hl: {
                break;
            }
            case Opcode::LD_0xFF00_plus_c_a: {
                break;
            }
            case Opcode::PUSH_hl: {
                break;
            }
            case Opcode::AND_a_u8: {
                break;
            }
            case Opcode::RST_0x20: {
                break;
            }
            case Opcode::ADD_sp_i8: {
                add_sp_e8(this, static_cast<i8>(this->bus_read_byte(reg.pc++)));
                break;
            }
            case Opcode::JP_hl: {
                break;
            }
            case Opcode::LD_u16_ptr_a: {
                break;
            }
            case Opcode::XOR_a_u8: {
                break;
            }
            case Opcode::RST_0x28: {
                break;
            }
            case Opcode::LD_a_0xFF00_plus_u8: {
                break;
            }
            case Opcode::POP_af: {
                break;
            }
            case Opcode::LD_a_0xFF00_plus_c: {
                break;
            }
            case Opcode::DI: {
                break;
            }
            case Opcode::PUSH_af: {
                break;
            }
            case Opcode::OR_a_u8: {
                break;
            }
            case Opcode::RST_0x30: {
                break;
            }
            case Opcode::LD_hl_sp_plus_i8: {
                break;
            }
            case Opcode::LD_sp_hl: {
                break;
            }
            case Opcode::LD_a_u16_ptr: {
                break;
            }
            case Opcode::EI: {
                break;
            }
            case Opcode::CP_a_u8: {
                break;
            }
            case Opcode::RST_0x38: {
                break;
            }
        }
    }

    void CPU::run_cycle() noexcept {
        u8 data = this->bus_read_byte(reg.pc++);
        this->dexec(data);
    }
}  // namespace mina::dmg
