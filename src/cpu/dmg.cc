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

// TODO(luiz): implement cycle timing correctness.

namespace mina::dmg {
    namespace {
        ///////////////////////////////////////////////////////////////////////
        // Registers.
        ///////////////////////////////////////////////////////////////////////

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

        /// Register file flags.
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

        /// Conditional execution.
        ///
        /// Execute the instruction if the corresponding register flag is set.
        enum class Cond {
            Z,   ///< Z flag set.
            C,   ///< C flag set.
            NZ,  ///< Z flag not set.
            NC,  ///< C flag not set.
        };

        /// 3-bit unsigned integer constant.
        struct UConst3 {
            u8 val     : 3 = 0b000;
            u8 unused_ : 5;
        };

        ///////////////////////////////////////////////////////////////////////
        // CPU instruction opcodes.
        ///////////////////////////////////////////////////////////////////////

        enum class Opcode : u8 {
            Nop                 = 0x00,
            LD_bc_u16           = 0x01,
            LD_bc_ptr_a         = 0x02,
            INC_bc              = 0x03,
            INC_b               = 0x04,
            DEC_b               = 0x05,
            LD_b_u8             = 0x06,
            RLCA                = 0x07,
            LD_u16_ptr_sp       = 0x08,
            ADD_hl_bc           = 0x09,
            LD_a_bc_ptr         = 0x0A,
            DEC_bc              = 0x0B,
            INC_c               = 0x0C,
            DEC_c               = 0x0D,
            LD_c_u8             = 0x0E,
            RRCA                = 0x0F,
            STOP                = 0x10,
            LD_de_u16           = 0x11,
            LD_de_ptr_a         = 0x12,
            INC_de              = 0x13,
            INC_d               = 0x14,
            DEC_d               = 0x15,
            LD_d_u8             = 0x16,
            RLA                 = 0x17,
            JR_i8               = 0x18,
            ADD_hl_de           = 0x19,
            LD_a_de_ptr         = 0x1A,
            DEC_de              = 0x1B,
            INC_e               = 0x1C,
            DEC_e               = 0x1D,
            LD_e_u8             = 0x1E,
            RRA                 = 0x1F,
            JR_nz_i8            = 0x20,
            LD_hl_u16           = 0x21,
            LD_high_hl_ptr_a    = 0x22,
            INC_hl              = 0x23,
            INC_h               = 0x24,
            DEC_h               = 0x25,
            LD_h_u8             = 0x26,
            DAA                 = 0x27,
            JR_z_i8             = 0x28,
            ADD_hl_hl           = 0x29,
            LD_a_high_hl_ptr    = 0x2A,
            DEC_hl              = 0x2B,
            INC_l               = 0x2C,
            DEC_l               = 0x2D,
            LD_l_u8             = 0x2E,
            CPL                 = 0x2F,
            JR_nc_i8            = 0x30,
            LD_sp_u16           = 0x31,
            LD_low_hl_ptr_a     = 0x32,
            INC_sp              = 0x33,
            INC_hl_ptr          = 0x34,
            DEC_hl_ptr          = 0x35,
            LD_hl_ptr_u8        = 0x36,
            SCF                 = 0x37,
            JR_c_i8             = 0x38,
            ADD_hl_sp           = 0x39,
            LD_a_low_hl_ptr     = 0x3A,
            DEC_sp              = 0x3B,
            INC_a               = 0x3C,
            DEC_a               = 0x3D,
            LD_a_u8             = 0x3E,
            CCF                 = 0x3F,
            LD_b_b              = 0x40,
            LD_b_c              = 0x41,
            LD_b_d              = 0x42,
            LD_b_e              = 0x43,
            LD_b_h              = 0x44,
            LD_b_l              = 0x45,
            LD_b_hl_ptr         = 0x46,
            LD_b_a              = 0x47,
            LD_c_b              = 0x48,
            LD_c_c              = 0x49,
            LD_c_d              = 0x4A,
            LD_c_e              = 0x4B,
            LD_c_h              = 0x4C,
            LD_c_l              = 0x4D,
            LD_c_hl_ptr         = 0x4E,
            LD_c_a              = 0x4F,
            LD_d_b              = 0x50,
            LD_d_c              = 0x51,
            LD_d_d              = 0x52,
            LD_d_e              = 0x53,
            LD_d_h              = 0x54,
            LD_d_l              = 0x55,
            LD_d_hl_ptr         = 0x56,
            LD_d_a              = 0x57,
            LD_e_b              = 0x58,
            LD_e_c              = 0x59,
            LD_e_d              = 0x5A,
            LD_e_e              = 0x5B,
            LD_e_h              = 0x5C,
            LD_e_l              = 0x5D,
            LD_e_hl_ptr         = 0x5E,
            LD_e_a              = 0x5F,
            LD_h_b              = 0x60,
            LD_h_c              = 0x61,
            LD_h_d              = 0x62,
            LD_h_e              = 0x63,
            LD_h_h              = 0x64,
            LD_h_l              = 0x65,
            LD_h_hl_ptr         = 0x66,
            LD_h_a              = 0x67,
            LD_l_b              = 0x68,
            LD_l_c              = 0x69,
            LD_l_d              = 0x6A,
            LD_l_e              = 0x6B,
            LD_l_h              = 0x6C,
            LD_l_l              = 0x6D,
            LD_l_hl_ptr         = 0x6E,
            LD_l_a              = 0x6F,
            LD_hl_ptr_b         = 0x70,
            LD_hl_ptr_c         = 0x71,
            LD_hl_ptr_d         = 0x72,
            LD_hl_ptr_e         = 0x73,
            LD_hl_ptr_h         = 0x74,
            LD_hl_ptr_l         = 0x75,
            HALT                = 0x76,
            LD_hl_ptr_a         = 0x77,
            LD_a_b              = 0x78,
            LD_a_c              = 0x79,
            LD_a_d              = 0x7A,
            LD_a_e              = 0x7B,
            LD_a_h              = 0x7C,
            LD_a_l              = 0x7D,
            LD_a_hl_ptr         = 0x7E,
            LD_a_a              = 0x7F,
            ADD_a_b             = 0x80,
            ADD_a_c             = 0x81,
            ADD_a_d             = 0x82,
            ADD_a_e             = 0x83,
            ADD_a_h             = 0x84,
            ADD_a_l             = 0x85,
            ADD_a_hl_ptr        = 0x86,
            ADD_a_a             = 0x87,
            ADC_a_b             = 0x88,
            ADC_a_c             = 0x89,
            ADC_a_d             = 0x8A,
            ADC_a_e             = 0x8B,
            ADC_a_h             = 0x8C,
            ADC_a_l             = 0x8D,
            ADC_a_hl            = 0x8E,
            ADC_a_a             = 0x8F,
            SUB_a_b             = 0x90,
            SUB_a_c             = 0x91,
            SUB_a_d             = 0x92,
            SUB_a_e             = 0x93,
            SUB_a_h             = 0x94,
            SUB_a_l             = 0x95,
            SUB_a_hl_ptr        = 0x96,
            SUB_a_a             = 0x97,
            SBC_a_b             = 0x98,
            SBC_a_c             = 0x99,
            SBC_a_d             = 0x9A,
            SBC_a_e             = 0x9B,
            SBC_a_h             = 0x9C,
            SBC_a_l             = 0x9D,
            SBC_a_hl_ptr        = 0x9E,
            SBC_a_a             = 0x9F,
            AND_a_b             = 0xA0,
            AND_a_c             = 0xA1,
            AND_a_d             = 0xA2,
            AND_a_e             = 0xA3,
            AND_a_h             = 0xA4,
            AND_a_l             = 0xA5,
            AND_a_hl_ptr        = 0xA6,
            AND_a_a             = 0xA7,
            XOR_a_b             = 0xA8,
            XOR_a_c             = 0xA9,
            XOR_a_d             = 0xAA,
            XOR_a_e             = 0xAB,
            XOR_a_h             = 0xAC,
            XOR_a_l             = 0xAD,
            XOR_a_hl_ptr        = 0xAE,
            XOR_a_a             = 0xAF,
            OR_a_b              = 0xB0,
            OR_a_c              = 0xB1,
            OR_a_d              = 0xB2,
            OR_a_e              = 0xB3,
            OR_a_h              = 0xB4,
            OR_a_l              = 0xB5,
            OR_a_hl_ptr         = 0xB6,
            OR_a_a              = 0xB7,
            CP_a_b              = 0xB8,
            CP_a_c              = 0xB9,
            CP_a_d              = 0xBA,
            CP_a_e              = 0xBB,
            CP_a_h              = 0xBC,
            CP_a_l              = 0xBD,
            CP_a_hl_ptr         = 0xBE,
            CP_a_a              = 0xBF,
            RET_nz              = 0xC0,
            POP_bc              = 0xC1,
            JP_zn_u16           = 0xC2,
            JP_u16              = 0xC3,
            CALL_nz_u16         = 0xC4,
            PUSH_bc             = 0xC5,
            ADD_a_u8            = 0xC6,
            RST_0x00            = 0xC7,
            RET_z               = 0xC8,
            RET                 = 0xC9,
            JP_z_u16            = 0xCA,
            PREFIX_0xCB         = 0xCB,
            CALL_z_u16          = 0xCC,
            CALL_u16            = 0xCD,
            ADC_a_u8            = 0xCE,
            RST_0x08            = 0xCF,
            RET_nc              = 0xD0,
            POP_de              = 0xD1,
            JP_zc_u16           = 0xD2,
            // Not defined (0xD3)
            CALL_nc_u16         = 0xD4,
            PUSH_de             = 0xD5,
            SUB_a_u8            = 0xD6,
            RST_0x10            = 0xD7,
            RET_c               = 0xD8,
            RETI                = 0xD9,
            JP_c_u16            = 0xDA,
            // Not defined (0xDB)
            CALL_c_u16          = 0xDC,
            // Not defined (0xDD)
            SBC_a_u8            = 0xDE,
            RST_0x18            = 0xDF,
            LD_0xFF00_plus_u8_a = 0xE0,
            POP_hl              = 0xE1,
            LD_0xFF00_plus_c_a  = 0xE2,
            // Not defined (0xE3)
            // Not defined (0xE4)
            PUSH_hl             = 0xE5,
            AND_a_u8            = 0xE6,
            RST_0x20            = 0xE7,
            ADD_sp_i8           = 0xE8,
            JP_hl               = 0xE9,
            LD_u16_ptr_a        = 0xEA,
            // Not defined (0xEB)
            // Not defined (0xEC)
            // Not defined (0xED)
            XOR_a_u8            = 0xEE,
            RST_0x28            = 0xEF,
            LD_a_0xFF00_plus_u8 = 0xF0,
            POP_af              = 0xF1,
            LD_a_0xFF00_plus_c  = 0xF2,
            DI                  = 0xF3,
            // Not defined (0xF4)
            PUSH_af             = 0xF5,
            OR_a_u8             = 0xF6,
            RST_0x30            = 0xF7,
            LD_hl_sp_plus_i8    = 0xF8,
            LD_sp_hl            = 0xF9,
            LD_a_u16_ptr        = 0xFA,
            EI                  = 0xFB,
            // Not defined (0xFC)
            // Not defined (0xFD)
            CP_a_u8             = 0xFE,
            RST_0x38            = 0xFF,
            // TODO: CB prefixed opcodes
        };

        ///////////////////////////////////////////////////////////////////////
        // Operations requested by the CPU.
        ///////////////////////////////////////////////////////////////////////

        u8 read_reg8(RegisterFile& regfile, Reg8 r) noexcept {
            u8 val;
            switch (r) {
                case Reg8::A: val = static_cast<u8>(regfile.af >> 8); break;
                case Reg8::B: val = static_cast<u8>(regfile.bc >> 8); break;
                case Reg8::C: val = static_cast<u8>(regfile.bc & 0x00FF); break;
                case Reg8::D: val = static_cast<u8>(regfile.de >> 8); break;
                case Reg8::E: val = static_cast<u8>(regfile.de & 0x00FF); break;
                case Reg8::H: val = static_cast<u8>(regfile.hl >> 8); break;
                case Reg8::L: val = static_cast<u8>(regfile.hl & 0x00FF); break;
            }
            return val;
        }

        u16 read_reg16(RegisterFile& regfile, Reg16 r) noexcept {
            u16 val;
            switch (r) {
                case Reg16::BC: val = regfile.bc; break;
                case Reg16::DE: val = regfile.de; break;
                case Reg16::HL: val = regfile.hl; break;
                case Reg16::SP: val = regfile.sp; break;
            }
            return val;
        }

        void set_reg8(RegisterFile& regfile, Reg8 r, u8 val) noexcept {
            psh_todo();
            switch (r) {
                case Reg8::A:
                case Reg8::B:
                case Reg8::C:
                case Reg8::D:
                case Reg8::E:
                case Reg8::H:
                case Reg8::L:
            }
        }

        void set_reg16(RegisterFile& regfile, Reg16 r, u16 val) noexcept {
            psh_todo();
            switch (r) {
                case Reg16::BC:
                case Reg16::DE:
                case Reg16::HL:
                case Reg16::SP:
            }
        }

        u8 read_flag(RegisterFile& regfile, Flag f) noexcept {
            u8 flags = regfile.af & 0x00FF;
            u8 val;
            switch (f) {
                case Flag::C: val = static_cast<u8>(flags << 4); break;
                case Flag::H: val = static_cast<u8>(flags << 5); break;
                case Flag::N: val = static_cast<u8>(flags << 6); break;
                case Flag::Z: val = static_cast<u8>(flags << 7); break;
            }
            return val;
        }

        void set_flag(RegisterFile& regfile, Flag f) noexcept {
            switch (f) {
                case Flag::C: regfile.af |= psh::bit(4); break;
                case Flag::H: regfile.af |= psh::bit(5); break;
                case Flag::N: regfile.af |= psh::bit(6); break;
                case Flag::Z: regfile.af |= psh::bit(7); break;
            }
        }

        void clear_flag(RegisterFile& regfile, Flag f) noexcept {
            switch (f) {
                case Flag::C: regfile.af &= psh::clear_bit(4); break;
                case Flag::H: regfile.af &= psh::clear_bit(5); break;
                case Flag::N: regfile.af &= psh::clear_bit(6); break;
                case Flag::Z: regfile.af &= psh::clear_bit(7); break;
            }
        }

        void set_or_clear_flag_if(RegisterFile& regfile, Flag f, bool cond) noexcept {
            auto const icond = static_cast<i32>(cond);
            switch (f) {
                case Flag::C: regfile.af ^= ((-icond ^ regfile.af) & psh::bit(4)); break;
                case Flag::H: regfile.af ^= ((-icond ^ regfile.af) & psh::bit(5)); break;
                case Flag::N: regfile.af ^= ((-icond ^ regfile.af) & psh::bit(6)); break;
                case Flag::Z: regfile.af ^= ((-icond ^ regfile.af) & psh::bit(7)); break;
            }
        }

        void clear_all_flags(RegisterFile& regfile) noexcept {
            regfile.af &= 0xFF00;
        }

        u8 bus_read_byte(CPU* cpu, u16 addr) noexcept {
            u8 const* memory = reinterpret_cast<u8 const*>(&cpu->mmap);
            cpu->bus_addr    = addr;
            return memory[cpu->bus_addr];
        }

        u8 bus_read_imm8(CPU* cpu) noexcept {
            u8 const* memory = reinterpret_cast<u8 const*>(&cpu->mmap);
            u8 const  bt     = memory[cpu->regfile.pc++];
            cpu->bus_addr    = cpu->regfile.pc;
            return bt;
        }

        u16 bus_read_imm16(CPU* cpu) noexcept {
            u8 const* memory = reinterpret_cast<u8 const*>(&cpu->mmap);
            u8 const  lo     = memory[cpu->regfile.pc++];
            u8 const  hi     = memory[cpu->regfile.pc++];
            cpu->bus_addr    = cpu->regfile.pc;
            return psh::make_u16(hi, lo);
        }

        ///////////////////////////////////////////////////////////////////////
        // Instructions requested by CPU.
        ///////////////////////////////////////////////////////////////////////

        // Load the immediate 16-bit value to a 16-bit register.
        void ld_r16_imm16(CPU* cpu, Reg16 reg, u16 imm) noexcept {
            set_reg16(cpu->regfile, reg, imm);
            psh_todo();
        }

        // Add to HL the value loaded in a 16-bit register.
        void add_hl_r16(CPU* cpu, Reg16 reg) noexcept {
            u16 const val = read_reg16(cpu->regfile, reg);
            u16 const hl  = read_reg16(cpu->regfile, Reg16::HL);

            u32 const res   = hl + val;
            cpu->regfile.hl = static_cast<u16>(res);

            cpu->regfile.af &= 0xFF00;  // Clear all flags.
            set_or_clear_flag_if(cpu->regfile, Flag::C, res > 0x0000FFFF);
            set_or_clear_flag_if(cpu->regfile, Flag::H, ((hl & 0x00FF) + (val & 0x00FF)) > 0x00FF);
        }

        // Add an 8-bit value to the accumulator register.
        void add(CPU* cpu, u8 val) noexcept {
            u8 const acc = read_reg8(cpu->regfile, Reg8::A);

            u16 const res   = acc + val;
            cpu->regfile.af = psh::as_high_u16(static_cast<u8>(res));  // All flags clear.

            set_or_clear_flag_if(cpu->regfile, Flag::C, res > 0x00FF);
            set_or_clear_flag_if(cpu->regfile, Flag::H, ((acc & 0x0F) + (val & 0x0F)) > 0x0F);
            set_or_clear_flag_if(cpu->regfile, Flag::Z, res == 0);
        }

        // Add an 8-bit integer offset to the stack pointer register.
        void add_sp_e8(CPU* cpu, i8 offset) noexcept {
            auto const res  = static_cast<u16>(psh_lb_add(
                static_cast<i16>(cpu->regfile.sp),
                static_cast<i16>(offset),
                static_cast<i16>(0)));
            cpu->regfile.sp = res;

            cpu->regfile.af &= 0xFF00;  // Clear all flags.
            set_or_clear_flag_if(cpu->regfile, Flag::C, res > 0x0000FFFF);
            set_or_clear_flag_if(
                cpu->regfile,
                Flag::H,
                ((cpu->regfile.sp & 0x00FF) + offset) > 0x00FF);
        }

        // Add an 8-bit value to the accumulator register together with the carry flag.
        void adc(CPU* cpu, u8 val) noexcept {
            u8 const acc   = read_reg8(cpu->regfile, Reg8::A);
            u8 const carry = read_flag(cpu->regfile, Flag::C);

            u16 const res   = acc + val + carry;
            cpu->regfile.af = psh::as_high_u16(static_cast<u8>(res));  // All flags clear.

            set_or_clear_flag_if(cpu->regfile, Flag::C, res > 0x00FF);
            set_or_clear_flag_if(
                cpu->regfile,
                Flag::H,
                ((acc & 0x0F) + (val & 0x0F) + carry) > 0x0F);
            set_or_clear_flag_if(cpu->regfile, Flag::Z, res == 0);
        }

        /// Jump the program counter register to the immediate 16-bit value address.
        void jp_u16(CPU* cpu, u16 addr) noexcept {
            cpu->regfile.pc = addr;
        }

        ///////////////////////////////////////////////////////////////////////
        // Decode and execute instructions.
        ///////////////////////////////////////////////////////////////////////

        void dexec(CPU* cpu, u8 data) noexcept {
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
                    add_hl_r16(cpu, Reg16::BC);
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
                    add_hl_r16(cpu, Reg16::DE);
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
                    add_hl_r16(cpu, Reg16::HL);
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
                    add_hl_r16(cpu, Reg16::SP);
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
                    add(cpu, read_reg8(cpu->regfile, Reg8::B));
                    break;
                }
                case Opcode::ADD_a_c: {
                    add(cpu, read_reg8(cpu->regfile, Reg8::C));
                    break;
                }
                case Opcode::ADD_a_d: {
                    add(cpu, read_reg8(cpu->regfile, Reg8::D));
                    break;
                }
                case Opcode::ADD_a_e: {
                    add(cpu, read_reg8(cpu->regfile, Reg8::E));
                    break;
                }
                case Opcode::ADD_a_h: {
                    add(cpu, read_reg8(cpu->regfile, Reg8::H));
                    break;
                }
                case Opcode::ADD_a_l: {
                    add(cpu, read_reg8(cpu->regfile, Reg8::L));
                    break;
                }
                case Opcode::ADD_a_hl_ptr: {
                    add(cpu, bus_read_byte(cpu, read_reg16(cpu->regfile, Reg16::HL)));
                    break;
                }
                case Opcode::ADD_a_a: {
                    add(cpu, read_reg8(cpu->regfile, Reg8::A));
                    break;
                }
                case Opcode::ADC_a_b: {
                    adc(cpu, read_reg8(cpu->regfile, Reg8::B));
                    break;
                }
                case Opcode::ADC_a_c: {
                    adc(cpu, read_reg8(cpu->regfile, Reg8::C));
                    break;
                }
                case Opcode::ADC_a_d: {
                    adc(cpu, read_reg8(cpu->regfile, Reg8::D));
                    break;
                }
                case Opcode::ADC_a_e: {
                    adc(cpu, read_reg8(cpu->regfile, Reg8::E));
                    break;
                }
                case Opcode::ADC_a_h: {
                    adc(cpu, read_reg8(cpu->regfile, Reg8::H));
                    break;
                }
                case Opcode::ADC_a_l: {
                    adc(cpu, read_reg8(cpu->regfile, Reg8::L));
                    break;
                }
                case Opcode::ADC_a_hl: {
                    adc(cpu, bus_read_byte(cpu, read_reg16(cpu->regfile, Reg16::HL)));
                    break;
                }
                case Opcode::ADC_a_a: {
                    adc(cpu, read_reg8(cpu->regfile, Reg8::A));
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
                    jp_u16(cpu, bus_read_imm16(cpu));
                    break;
                }
                case Opcode::CALL_nz_u16: {
                    break;
                }
                case Opcode::PUSH_bc: {
                    break;
                }
                case Opcode::ADD_a_u8: {
                    add(cpu, bus_read_imm8(cpu));
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
                    adc(cpu, bus_read_imm8(cpu));
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
                    add_sp_e8(cpu, static_cast<i8>(bus_read_byte(cpu, cpu->regfile.pc++)));
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
    }  // namespace

    void CPU::run_cycle() noexcept {
        u8 data = bus_read_byte(this, regfile.pc++);
        dexec(this, data);
    }
}  // namespace mina::dmg
