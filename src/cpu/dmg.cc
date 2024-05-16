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
#include <psh/bit.h>
#include <psh/intrinsics.h>

// TODO(luiz): implement cycle timing correctness.

////////////////////////////////////////////////////////////////////////////////
// Register file manipulation macros.
////////////////////////////////////////////////////////////////////////////////

namespace mina::dmg {
    namespace {
        ///////////////////////////////////////////////////////////////////////
        // Registers.
        ///////////////////////////////////////////////////////////////////////

        // TODO(luiz): deal with Reg8::HL_ptr distinctly for every instruction depending on Reg8.
        //             You can do this by casing Reg8::HL_ptr at the top, which prevents it from
        //             being caught by the fallthrough.

        /// 8-bit registers in little-endian order.
        enum class Reg8 : u8 {
            B      = 0x00,
            C      = 0x01,
            D      = 0x02,
            E      = 0x03,
            H      = 0x04,
            L      = 0x05,
            HL_ptr = 0x06,
            A      = 0x07,
        };

        /// 16-bit registers.
        enum class Reg16 : u8 {
            BC = 0x01,
            DE = 0x02,
            HL = 0x03,
            SP = 0x04,
        };

        /// Register file flags.
        enum class Flag : u8 {
            /// The carry flag. Set in the following occasions:
            ///     * 8-bit addition is higher than 0xFF.
            ///     * 16-bit addition is higher than 0xFFFF.
            ///     * Result of subtraction or comparison is negative.
            ///     * If a shift operation shifts out a 0b1 valued bit.
            C = 0x04,
            H = 0x05,  ///< Indicates carry for the high nibble.
            N = 0x06,  ///< If the last operation was a subtraction.
            Z = 0x07,  ///< If the last operation result was zero.
        };

        /// Conditional execution.
        ///
        /// Execute the instruction if the corresponding register flag is set.
        enum class Cond : u8 {
            NZ = 0x01,  ///< Z flag not set.
            Z  = 0x02,  ///< Z flag set.
            NC = 0x03,  ///< C flag not set.
            C  = 0x04,  ///< C flag set.
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
            Nop                = 0x00,
            LD_bc_u16          = 0x01,
            LD_bc_ptr_a        = 0x02,
            INC_bc             = 0x03,
            INC_b              = 0x04,
            DEC_b              = 0x05,
            LD_b_u8            = 0x06,
            RLCA               = 0x07,
            LD_u16_ptr_sp      = 0x08,
            ADD_hl_bc          = 0x09,
            LD_a_bc_ptr        = 0x0A,
            DEC_bc             = 0x0B,
            INC_c              = 0x0C,
            DEC_c              = 0x0D,
            LD_c_u8            = 0x0E,
            RRCA               = 0x0F,
            STOP               = 0x10,
            LD_de_u16          = 0x11,
            LD_de_ptr_a        = 0x12,
            INC_de             = 0x13,
            INC_d              = 0x14,
            DEC_d              = 0x15,
            LD_d_u8            = 0x16,
            RLA                = 0x17,
            JR_i8              = 0x18,
            ADD_hl_de          = 0x19,
            LD_a_de_ptr        = 0x1A,
            DEC_de             = 0x1B,
            INC_e              = 0x1C,
            DEC_e              = 0x1D,
            LD_e_u8            = 0x1E,
            RRA                = 0x1F,
            JR_nz_i8           = 0x20,
            LD_hl_u16          = 0x21,
            LDI_hl_ptr_a       = 0x22,
            INC_hl             = 0x23,
            INC_h              = 0x24,
            DEC_h              = 0x25,
            LD_h_u8            = 0x26,
            DAA                = 0x27,
            JR_z_i8            = 0x28,
            ADD_hl_hl          = 0x29,
            LDI_a_hl_ptr       = 0x2A,
            DEC_hl             = 0x2B,
            INC_l              = 0x2C,
            DEC_l              = 0x2D,
            LD_l_u8            = 0x2E,
            CPL                = 0x2F,
            JR_nc_i8           = 0x30,
            LD_sp_u16          = 0x31,
            LDD_hl_ptr_a       = 0x32,
            INC_sp             = 0x33,
            INC_hl_ptr         = 0x34,
            DEC_hl_ptr         = 0x35,
            LD_hl_ptr_u8       = 0x36,
            SCF                = 0x37,
            JR_c_i8            = 0x38,
            ADD_hl_sp          = 0x39,
            LDD_a_hl_ptr       = 0x3A,
            DEC_sp             = 0x3B,
            INC_a              = 0x3C,
            DEC_a              = 0x3D,
            LD_a_u8            = 0x3E,
            CCF                = 0x3F,
            LD_b_b             = 0x40,
            LD_b_c             = 0x41,
            LD_b_d             = 0x42,
            LD_b_e             = 0x43,
            LD_b_h             = 0x44,
            LD_b_l             = 0x45,
            LD_b_hl_ptr        = 0x46,
            LD_b_a             = 0x47,
            LD_c_b             = 0x48,
            LD_c_c             = 0x49,
            LD_c_d             = 0x4A,
            LD_c_e             = 0x4B,
            LD_c_h             = 0x4C,
            LD_c_l             = 0x4D,
            LD_c_hl_ptr        = 0x4E,
            LD_c_a             = 0x4F,
            LD_d_b             = 0x50,
            LD_d_c             = 0x51,
            LD_d_d             = 0x52,
            LD_d_e             = 0x53,
            LD_d_h             = 0x54,
            LD_d_l             = 0x55,
            LD_d_hl_ptr        = 0x56,
            LD_d_a             = 0x57,
            LD_e_b             = 0x58,
            LD_e_c             = 0x59,
            LD_e_d             = 0x5A,
            LD_e_e             = 0x5B,
            LD_e_h             = 0x5C,
            LD_e_l             = 0x5D,
            LD_e_hl_ptr        = 0x5E,
            LD_e_a             = 0x5F,
            LD_h_b             = 0x60,
            LD_h_c             = 0x61,
            LD_h_d             = 0x62,
            LD_h_e             = 0x63,
            LD_h_h             = 0x64,
            LD_h_l             = 0x65,
            LD_h_hl_ptr        = 0x66,
            LD_h_a             = 0x67,
            LD_l_b             = 0x68,
            LD_l_c             = 0x69,
            LD_l_d             = 0x6A,
            LD_l_e             = 0x6B,
            LD_l_h             = 0x6C,
            LD_l_l             = 0x6D,
            LD_l_hl_ptr        = 0x6E,
            LD_l_a             = 0x6F,
            LD_hl_ptr_b        = 0x70,
            LD_hl_ptr_c        = 0x71,
            LD_hl_ptr_d        = 0x72,
            LD_hl_ptr_e        = 0x73,
            LD_hl_ptr_h        = 0x74,
            LD_hl_ptr_l        = 0x75,
            HALT               = 0x76,
            LD_hl_ptr_a        = 0x77,
            LD_a_b             = 0x78,
            LD_a_c             = 0x79,
            LD_a_d             = 0x7A,
            LD_a_e             = 0x7B,
            LD_a_h             = 0x7C,
            LD_a_l             = 0x7D,
            LD_a_hl_ptr        = 0x7E,
            LD_a_a             = 0x7F,
            ADD_a_b            = 0x80,
            ADD_a_c            = 0x81,
            ADD_a_d            = 0x82,
            ADD_a_e            = 0x83,
            ADD_a_h            = 0x84,
            ADD_a_l            = 0x85,
            ADD_a_hl_ptr       = 0x86,
            ADD_a_a            = 0x87,
            ADC_a_b            = 0x88,
            ADC_a_c            = 0x89,
            ADC_a_d            = 0x8A,
            ADC_a_e            = 0x8B,
            ADC_a_h            = 0x8C,
            ADC_a_l            = 0x8D,
            ADC_a_hl_ptr       = 0x8E,
            ADC_a_a            = 0x8F,
            SUB_a_b            = 0x90,
            SUB_a_c            = 0x91,
            SUB_a_d            = 0x92,
            SUB_a_e            = 0x93,
            SUB_a_h            = 0x94,
            SUB_a_l            = 0x95,
            SUB_a_hl_ptr       = 0x96,
            SUB_a_a            = 0x97,
            SBC_a_b            = 0x98,
            SBC_a_c            = 0x99,
            SBC_a_d            = 0x9A,
            SBC_a_e            = 0x9B,
            SBC_a_h            = 0x9C,
            SBC_a_l            = 0x9D,
            SBC_a_hl_ptr       = 0x9E,
            SBC_a_a            = 0x9F,
            AND_a_b            = 0xA0,
            AND_a_c            = 0xA1,
            AND_a_d            = 0xA2,
            AND_a_e            = 0xA3,
            AND_a_h            = 0xA4,
            AND_a_l            = 0xA5,
            AND_a_hl_ptr       = 0xA6,
            AND_a_a            = 0xA7,
            XOR_a_b            = 0xA8,
            XOR_a_c            = 0xA9,
            XOR_a_d            = 0xAA,
            XOR_a_e            = 0xAB,
            XOR_a_h            = 0xAC,
            XOR_a_l            = 0xAD,
            XOR_a_hl_ptr       = 0xAE,
            XOR_a_a            = 0xAF,
            OR_a_b             = 0xB0,
            OR_a_c             = 0xB1,
            OR_a_d             = 0xB2,
            OR_a_e             = 0xB3,
            OR_a_h             = 0xB4,
            OR_a_l             = 0xB5,
            OR_a_hl_ptr        = 0xB6,
            OR_a_a             = 0xB7,
            CP_a_b             = 0xB8,
            CP_a_c             = 0xB9,
            CP_a_d             = 0xBA,
            CP_a_e             = 0xBB,
            CP_a_h             = 0xBC,
            CP_a_l             = 0xBD,
            CP_a_hl_ptr        = 0xBE,
            CP_a_a             = 0xBF,
            RET_nz             = 0xC0,
            POP_bc             = 0xC1,
            JP_nz_u16          = 0xC2,
            JP_u16             = 0xC3,
            CALL_nz_u16        = 0xC4,
            PUSH_bc            = 0xC5,
            ADD_a_u8           = 0xC6,
            RST_0x00           = 0xC7,
            RET_z              = 0xC8,
            RET                = 0xC9,
            JP_z_u16           = 0xCA,
            PREFIX_0xCB        = 0xCB,
            CALL_z_u16         = 0xCC,
            CALL_u16           = 0xCD,
            ADC_a_u8           = 0xCE,
            RST_0x08           = 0xCF,
            RET_nc             = 0xD0,
            POP_de             = 0xD1,
            JP_nc_u16          = 0xD2,
            CALL_nc_u16        = 0xD4,
            PUSH_de            = 0xD5,
            SUB_a_u8           = 0xD6,
            RST_0x10           = 0xD7,
            RET_c              = 0xD8,
            RETI               = 0xD9,
            JP_c_u16           = 0xDA,
            CALL_c_u16         = 0xDC,
            SBC_a_u8           = 0xDE,
            RST_0x18           = 0xDF,
            LDH_u16_ptr_a      = 0xE0,
            POP_hl             = 0xE1,
            LD_0xFF00_plus_c_a = 0xE2,
            PUSH_hl            = 0xE5,
            AND_a_u8           = 0xE6,
            RST_0x20           = 0xE7,
            ADD_sp_i8          = 0xE8,
            JP_hl              = 0xE9,
            LD_u16_ptr_a       = 0xEA,
            XOR_a_u8           = 0xEE,
            RST_0x28           = 0xEF,
            LDH_a_u16_ptr      = 0xF0,
            POP_af             = 0xF1,
            LD_a_0xFF00_plus_c = 0xF2,
            DI                 = 0xF3,
            PUSH_af            = 0xF5,
            OR_a_u8            = 0xF6,
            RST_0x30           = 0xF7,
            LD_hl_sp_plus_i8   = 0xF8,
            LD_sp_hl           = 0xF9,
            LD_a_u16_ptr       = 0xFA,
            EI                 = 0xFB,
            CP_a_u8            = 0xFE,
            RST_0x38           = 0xFF,
        };

        enum class CBOpcode {
            // TODO(luiz): CB prefixed opcodes
        };

        ///////////////////////////////////////////////////////////////////////
        // Memory operations.
        ///////////////////////////////////////////////////////////////////////

#define mina_cpu_memory(cpu) reinterpret_cast<u8*>(&cpu->mmap)

#define mina_mmap_write_byte(cpu, dst_addr, val_u8)  \
    do {                                             \
        *(mina_cpu_memory(cpu) + dst_addr) = val_u8; \
    } while (0)

#define mina_mmap_write_word(cpu, dst_addr, val_u16)              \
    do {                                                          \
        u16 addr__                           = (dst_addr);        \
        u16 val__                            = (val_u16);         \
        *(mina_cpu_memory(cpu) + addr__)     = psh_u16_lo(val__); \
        *(mina_cpu_memory(cpu) + addr__ + 1) = psh_u16_hi(val__); \
    } while (0)

        u8 bus_read_byte(CPU* cpu, u16 addr) noexcept {
            u8 const* memory = mina_cpu_memory(cpu);
            cpu->bus_addr    = addr;
            return memory[cpu->bus_addr];
        }

        u8 bus_read_imm8(CPU* cpu) noexcept {
            u8 const* memory = mina_cpu_memory(cpu);
            u8        bt     = memory[cpu->regfile.pc++];
            cpu->bus_addr    = cpu->regfile.pc;
            return bt;
        }

        u16 bus_read_imm16(CPU* cpu) noexcept {
            u8 const* memory = mina_cpu_memory(cpu);
            u8        lo     = memory[cpu->regfile.pc++];
            u8        hi     = memory[cpu->regfile.pc++];
            cpu->bus_addr    = cpu->regfile.pc;
            return psh_u16_from_bytes(hi, lo);
        }

        ///////////////////////////////////////////////////////////////////////
        // Register file operations.
        ///////////////////////////////////////////////////////////////////////

#define mina_cpu_regfile_memory(cpu) reinterpret_cast<u8*>(&cpu->regfile)

#define mina_read_reg16(cpu, reg16)                                   \
    psh_u16_from_bytes(                                               \
        *(mina_cpu_regfile_memory(cpu) + static_cast<u8>(reg16) + 1), \
        *(mina_cpu_regfile_memory(cpu) + static_cast<u8>(reg16)))

#define mina_set_reg16(cpu, reg16, val_u16)                                   \
    do {                                                                      \
        u16 val__    = (val_u16);                                             \
        u8* reg__    = mina_cpu_regfile_memory(cpu) + static_cast<u8>(reg16); \
        *(reg__)     = static_cast<u8>(psh_u16_lo(val__));                    \
        *(reg__ + 1) = static_cast<u8>(psh_u16_hi(val__));                    \
    } while (0)

        // NOTE(luiz): Things are a bit weird with weird with 8-bit registers due to the convention
        //             of the SM83 CPU opcodes for solving which 8-bit register is being referred.
        //             The ordering is as in `Reg8`, which doesn't reflect the memory layout of the
        //             register file. It's bad but what can I do about it...

        u8 read_reg8(CPU* cpu, Reg8 reg) noexcept {
            u8 val;
            if (reg == Reg8::HL_ptr) {
                val = bus_read_byte(cpu, mina_read_reg16(cpu, Reg16::HL));
            } else if (reg == Reg8::A) {
                val = cpu->regfile.a;
            } else {
                val = *(mina_cpu_regfile_memory(cpu) + static_cast<u8>(reg));
            }
            return val;
        }

        void set_reg8(CPU* cpu, Reg8 reg, u8 val) noexcept {
            if (reg == Reg8::HL_ptr) {
                u8* memory       = mina_cpu_memory(cpu);
                u16 addr         = mina_read_reg16(cpu, Reg16::HL);
                *(memory + addr) = val;
            } else if (reg == Reg8::A) {
                cpu->regfile.a = val;
            } else {
                *(mina_cpu_regfile_memory(cpu) + static_cast<u8>(reg)) = val;
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // Register flag operations.
        ///////////////////////////////////////////////////////////////////////

#define mina_read_flag(cpu, flag)  psh_bit_at(cpu->regfile.f, static_cast<u8>(flag))
#define mina_set_flag(cpu, flag)   psh_bit_set(cpu->regfile.f, static_cast<u8>(flag))
#define mina_clear_flag(cpu, flag) psh_bit_clear(cpu->regfile.f, static_cast<u8>(flag))
#define mina_set_or_clear_flag_if(cpu, flag, cond) \
    psh_bit_set_or_clear_if(cpu->regfile.f, static_cast<u8>(flag), cond)

#define mina_clear_all_flags(regfile) \
    do {                              \
        regfile.f = 0x00;             \
    } while (0)

        bool read_condition_flag(CPU* cpu, Cond cc) {
            bool res;
            switch (cc) {
                case Cond::NZ: res = !static_cast<bool>(mina_read_flag(cpu, Flag::Z)); break;
                case Cond::Z:  res = static_cast<bool>(mina_read_flag(cpu, Flag::Z)); break;
                case Cond::NC: res = !static_cast<bool>(mina_read_flag(cpu, Flag::C)); break;
                case Cond::C:  res = static_cast<bool>(mina_read_flag(cpu, Flag::C)); break;
            }
            return res;
        }

        ///////////////////////////////////////////////////////////////////////
        // Decode and execute instructions.
        ///////////////////////////////////////////////////////////////////////

        void dexec(CPU* cpu, u8 data) noexcept {
            auto opcode = static_cast<Opcode>(data);
            u8   x      = psh_bits_at(data, 7, 2);
            u8   y      = psh_bits_at(data, 4, 3);
            u8   p      = psh_bits_at(y, 2, 2);
            u8   q      = psh_bits_at(y, 1, 1);
            u8   z      = psh_bits_at(data, 1, 3);

            switch (opcode) {
                case Opcode::Nop: {
                    break;
                }

                case Opcode::HALT: {  // NOTE(luiz): Only exception, load [HL] [HL] causes a halt.
                                      // WARNING(luiz): Don't change its position.
                    psh_todo_msg("HALT");
                }
                case Opcode::LD_b_b:
                case Opcode::LD_b_c:
                case Opcode::LD_b_d:
                case Opcode::LD_b_e:
                case Opcode::LD_b_h:
                case Opcode::LD_b_l:
                case Opcode::LD_b_hl_ptr:
                case Opcode::LD_b_a:
                case Opcode::LD_c_b:
                case Opcode::LD_c_c:
                case Opcode::LD_c_d:
                case Opcode::LD_c_e:
                case Opcode::LD_c_h:
                case Opcode::LD_c_l:
                case Opcode::LD_c_hl_ptr:
                case Opcode::LD_c_a:
                case Opcode::LD_d_b:
                case Opcode::LD_d_c:
                case Opcode::LD_d_d:
                case Opcode::LD_d_e:
                case Opcode::LD_d_h:
                case Opcode::LD_d_l:
                case Opcode::LD_d_hl_ptr:
                case Opcode::LD_d_a:
                case Opcode::LD_e_b:
                case Opcode::LD_e_c:
                case Opcode::LD_e_d:
                case Opcode::LD_e_e:
                case Opcode::LD_e_h:
                case Opcode::LD_e_l:
                case Opcode::LD_e_hl_ptr:
                case Opcode::LD_e_a:
                case Opcode::LD_h_b:
                case Opcode::LD_h_c:
                case Opcode::LD_h_d:
                case Opcode::LD_h_e:
                case Opcode::LD_h_h:
                case Opcode::LD_h_l:
                case Opcode::LD_h_hl_ptr:
                case Opcode::LD_h_a:
                case Opcode::LD_l_b:
                case Opcode::LD_l_c:
                case Opcode::LD_l_d:
                case Opcode::LD_l_e:
                case Opcode::LD_l_h:
                case Opcode::LD_l_l:
                case Opcode::LD_l_hl_ptr:
                case Opcode::LD_l_a:
                case Opcode::LD_hl_ptr_b:
                case Opcode::LD_hl_ptr_c:
                case Opcode::LD_hl_ptr_d:
                case Opcode::LD_hl_ptr_e:
                case Opcode::LD_hl_ptr_h:
                case Opcode::LD_hl_ptr_l:
                case Opcode::LD_hl_ptr_a:
                case Opcode::LD_a_b:
                case Opcode::LD_a_c:
                case Opcode::LD_a_d:
                case Opcode::LD_a_e:
                case Opcode::LD_a_h:
                case Opcode::LD_a_l:
                case Opcode::LD_a_hl_ptr:
                case Opcode::LD_a_a:      {
                    set_reg8(cpu, static_cast<Reg8>(y), read_reg8(cpu, static_cast<Reg8>(z)));
                    break;
                }

                case Opcode::LD_b_u8:
                case Opcode::LD_c_u8:
                case Opcode::LD_d_u8:
                case Opcode::LD_e_u8:
                case Opcode::LD_h_u8:
                case Opcode::LD_l_u8:
                case Opcode::LD_hl_ptr_u8:
                case Opcode::LD_a_u8:      {
                    set_reg8(cpu, static_cast<Reg8>(y), bus_read_imm8(cpu));
                    break;
                }

                case Opcode::LD_bc_u16:
                case Opcode::LD_de_u16:
                case Opcode::LD_hl_u16:
                case Opcode::LD_sp_u16: {
                    mina_set_reg16(cpu, static_cast<Reg16>(p), bus_read_imm16(cpu));
                    break;
                }
                case Opcode::LD_sp_hl: {
                    mina_set_reg16(cpu, Reg16::SP, mina_read_reg16(cpu, Reg16::HL));
                    break;
                }

                case Opcode::LD_hl_sp_plus_i8: {
                    i8  offset = static_cast<i8>(bus_read_imm8(cpu));
                    u16 sp     = mina_read_reg16(cpu, Reg16::SP);
                    u32 res    = static_cast<u32>(sp + offset);
                    mina_set_reg16(cpu, Reg16::SP, static_cast<u16>(res));
                    mina_set_reg16(cpu, Reg16::HL, static_cast<u16>(res));

                    mina_clear_flag(cpu, Flag::Z);
                    mina_clear_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::C, res > 0xFFFF);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        static_cast<u16>(psh_u16_lo(sp) + offset) > 0xFF);
                    break;
                }

                case Opcode::LD_bc_ptr_a: {
                    mina_mmap_write_byte(cpu, mina_read_reg16(cpu, Reg16::BC), cpu->regfile.a);
                    break;
                }
                case Opcode::LD_u16_ptr_sp: {
                    mina_mmap_write_word(cpu, bus_read_imm16(cpu), mina_read_reg16(cpu, Reg16::SP));
                    break;
                }
                case Opcode::LD_u16_ptr_a: {
                    mina_mmap_write_byte(cpu, bus_read_imm16(cpu), cpu->regfile.a);
                    break;
                }
                case Opcode::LD_a_u16_ptr: {
                    cpu->regfile.a = bus_read_byte(cpu, bus_read_imm16(cpu));
                    break;
                }

                case Opcode::LDH_u16_ptr_a: {
                    u16 addr = bus_read_imm16(cpu);
                    if (0xFF00 <= addr) {
                        mina_mmap_write_byte(cpu, addr, cpu->regfile.a);
                    }
                    break;
                }
                case Opcode::LDH_a_u16_ptr: {
                    u16 addr = bus_read_imm16(cpu);
                    if (0xFF00 <= addr) {
                        u8 const* memory = mina_cpu_memory(cpu);
                        cpu->regfile.a   = *(memory + addr);
                    }
                    break;
                }

                case Opcode::LD_a_bc_ptr: {
                    cpu->regfile.a = bus_read_byte(cpu, mina_read_reg16(cpu, Reg16::BC));
                    break;
                }
                case Opcode::LD_a_de_ptr: {
                    cpu->regfile.a = bus_read_byte(cpu, mina_read_reg16(cpu, Reg16::DE));
                    break;
                }
                case Opcode::LD_de_ptr_a: {
                    mina_mmap_write_byte(cpu, mina_read_reg16(cpu, Reg16::DE), cpu->regfile.a);
                    break;
                }

                case Opcode::LDI_hl_ptr_a: {
                    u16 hl = mina_read_reg16(cpu, Reg16::HL);
                    mina_mmap_write_byte(cpu, hl, cpu->regfile.a);
                    mina_set_reg16(cpu, Reg16::HL, hl + 1);
                    break;
                }
                case Opcode::LDD_hl_ptr_a: {
                    u16 hl = mina_read_reg16(cpu, Reg16::HL);
                    mina_mmap_write_byte(cpu, hl, cpu->regfile.a);
                    mina_set_reg16(cpu, Reg16::HL, hl - 1);
                    break;
                }

                case Opcode::LDI_a_hl_ptr: {
                    u16 hl         = mina_read_reg16(cpu, Reg16::HL);
                    cpu->regfile.a = bus_read_byte(cpu, hl);
                    mina_set_reg16(cpu, Reg16::HL, hl + 1);
                    break;
                }
                case Opcode::LDD_a_hl_ptr: {
                    u16 hl         = mina_read_reg16(cpu, Reg16::HL);
                    cpu->regfile.a = bus_read_byte(cpu, hl);
                    mina_set_reg16(cpu, Reg16::HL, hl - 1);
                    break;
                }

                case Opcode::LD_0xFF00_plus_c_a: {
                    mina_mmap_write_byte(cpu, 0xFF00 + cpu->regfile.c, cpu->regfile.a);
                    break;
                }
                case Opcode::LD_a_0xFF00_plus_c: {
                    cpu->regfile.a = *(mina_cpu_memory(cpu) + 0xFF00 + cpu->regfile.c);
                    break;
                }

                case Opcode::JR_i8: {
                    i8 rel_addr = static_cast<i8>(bus_read_imm16(cpu));
                    cpu->regfile.pc += rel_addr;
                    break;
                }

                case Opcode::JR_nz_i8:
                case Opcode::JR_z_i8:
                case Opcode::JR_nc_i8:
                case Opcode::JR_c_i8:  {
                    Cond cc = static_cast<Cond>(y - 4);
                    if (read_condition_flag(cpu, cc)) {
                        i8 rel_addr = static_cast<i8>(bus_read_imm16(cpu));
                        cpu->regfile.pc += rel_addr;
                    }
                    break;
                }

                case Opcode::JP_hl: {
                    cpu->regfile.pc = mina_read_reg16(cpu, Reg16::HL);
                    break;
                }

                case Opcode::JP_u16: {
                    cpu->regfile.pc = bus_read_imm16(cpu);
                    break;
                }

                case Opcode::JP_nz_u16:
                case Opcode::JP_z_u16:
                case Opcode::JP_nc_u16:
                case Opcode::JP_c_u16:  {
                    Cond cc = static_cast<Cond>(y);
                    if (read_condition_flag(cpu, cc)) {
                        cpu->regfile.pc = bus_read_imm16(cpu);
                    }
                    break;
                }

                case Opcode::INC_b:
                case Opcode::INC_c:
                case Opcode::INC_d:
                case Opcode::INC_e:
                case Opcode::INC_h:
                case Opcode::INC_l:
                case Opcode::INC_hl_ptr:
                case Opcode::INC_a:      {
                    Reg8 reg      = static_cast<Reg8>(y);
                    u8   prev_val = read_reg8(cpu, reg);
                    set_reg8(cpu, reg, prev_val + 1);

                    mina_clear_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, prev_val + 1 == 0);
                    mina_set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) + 1 > 0x0F);
                    break;
                }

                case Opcode::INC_bc:
                case Opcode::INC_de:
                case Opcode::INC_hl:
                case Opcode::INC_sp: {
                    Reg16 reg = static_cast<Reg16>(p);
                    mina_set_reg16(cpu, reg, mina_read_reg16(cpu, reg) + 1);
                    break;
                }

                case Opcode::DEC_b:
                case Opcode::DEC_c:
                case Opcode::DEC_d:
                case Opcode::DEC_e:
                case Opcode::DEC_h:
                case Opcode::DEC_l:
                case Opcode::DEC_hl_ptr:
                case Opcode::DEC_a:      {
                    Reg8 reg      = static_cast<Reg8>(y);
                    u8   prev_val = read_reg8(cpu, reg);
                    set_reg8(cpu, reg, prev_val - 1);

                    mina_set_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, prev_val - 1 == 0);
                    mina_set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) - 1 > 0x0F);
                    break;
                }

                case Opcode::DEC_bc:
                case Opcode::DEC_de:
                case Opcode::DEC_hl:
                case Opcode::DEC_sp: {
                    Reg16 reg = static_cast<Reg16>(p);
                    mina_set_reg16(cpu, reg, mina_read_reg16(cpu, reg) - 1);
                    break;
                }

                case Opcode::ADD_a_b:
                case Opcode::ADD_a_c:
                case Opcode::ADD_a_d:
                case Opcode::ADD_a_e:
                case Opcode::ADD_a_h:
                case Opcode::ADD_a_l:
                case Opcode::ADD_a_hl_ptr:
                case Opcode::ADD_a_a:      {
                    Reg8 reg       = static_cast<Reg8>(z);
                    u8   val       = read_reg8(cpu, reg);
                    u8   acc       = cpu->regfile.a;
                    u16  res       = static_cast<u16>(acc + val);
                    cpu->regfile.a = static_cast<u8>(res);

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val)) > 0x0F);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    break;
                }

                case Opcode::ADD_a_u8: {
                    u8  val        = bus_read_imm8(cpu);
                    u8  acc        = cpu->regfile.a;
                    u16 res        = static_cast<u16>(acc + val);
                    cpu->regfile.a = static_cast<u8>(res);

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val)) > 0x0F);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    break;
                }

                case Opcode::ADD_sp_i8: {
                    i8  offset = static_cast<i8>(bus_read_imm8(cpu));
                    u16 sp     = mina_read_reg16(cpu, Reg16::SP);
                    u32 res    = static_cast<u32>(sp + offset);
                    mina_set_reg16(cpu, Reg16::SP, static_cast<u16>(res));

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::C, res > 0xFFFF);
                    mina_set_or_clear_flag_if(cpu, Flag::H, psh_u16_lo(sp) + offset > 0x00FF);
                    break;
                }

                case Opcode::ADD_hl_bc:
                case Opcode::ADD_hl_de:
                case Opcode::ADD_hl_hl:
                case Opcode::ADD_hl_sp: {
                    Reg16 reg = static_cast<Reg16>(p);
                    u16   val = mina_read_reg16(cpu, reg);
                    u16   hl  = mina_read_reg16(cpu, Reg16::HL);
                    mina_set_reg16(cpu, Reg16::HL, static_cast<u16>(hl + val));

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::C, hl + val > 0xFFFF);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        psh_u16_lo(hl) + psh_u16_lo(val) > 0x00FF);
                    break;
                }

                case Opcode::ADC_a_b:
                case Opcode::ADC_a_c:
                case Opcode::ADC_a_d:
                case Opcode::ADC_a_e:
                case Opcode::ADC_a_h:
                case Opcode::ADC_a_l:
                case Opcode::ADC_a_hl_ptr:
                case Opcode::ADC_a_a:      {
                    Reg8 reg       = static_cast<Reg8>(z);
                    u8   val       = read_reg8(cpu, reg);
                    u8   acc       = cpu->regfile.a;
                    u8   carry     = mina_read_flag(cpu, Flag::C);
                    u16  res       = static_cast<u16>(acc + val + carry);
                    cpu->regfile.a = static_cast<u8>(res);

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    mina_set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    break;
                }

                case Opcode::ADC_a_u8: {
                    u8  val        = bus_read_imm8(cpu);
                    u8  acc        = cpu->regfile.a;
                    u8  carry      = mina_read_flag(cpu, Flag::C);
                    u16 res        = static_cast<u16>(acc + val + carry);
                    cpu->regfile.a = static_cast<u8>(res);

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    mina_set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    break;
                }

                case Opcode::SUB_a_b:
                case Opcode::SUB_a_c:
                case Opcode::SUB_a_d:
                case Opcode::SUB_a_e:
                case Opcode::SUB_a_h:
                case Opcode::SUB_a_l:
                case Opcode::SUB_a_hl_ptr:
                case Opcode::SUB_a_a:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    u8   acc = cpu->regfile.a;
                    cpu->regfile.a -= val;

                    mina_set_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    mina_set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    mina_set_or_clear_flag_if(cpu, Flag::C, val > acc);
                    break;
                }

                case Opcode::SUB_a_u8: {
                    u8 val = bus_read_imm8(cpu);
                    u8 acc = cpu->regfile.a;
                    cpu->regfile.a -= val;

                    mina_set_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    mina_set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    mina_set_or_clear_flag_if(cpu, Flag::C, val > acc);
                    break;
                }

                case Opcode::SBC_a_b:
                case Opcode::SBC_a_c:
                case Opcode::SBC_a_d:
                case Opcode::SBC_a_e:
                case Opcode::SBC_a_h:
                case Opcode::SBC_a_l:
                case Opcode::SBC_a_hl_ptr:
                case Opcode::SBC_a_a:      {
                    Reg8 reg   = static_cast<Reg8>(z);
                    u8   val   = read_reg8(cpu, reg);
                    u8   acc   = cpu->regfile.a;
                    u8   carry = mina_read_flag(cpu, Flag::C);
                    cpu->regfile.a -= val - carry;

                    mina_set_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    mina_set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
                    break;
                }

                case Opcode::SBC_a_u8: {
                    u8 val   = bus_read_imm8(cpu);
                    u8 acc   = cpu->regfile.a;
                    u8 carry = mina_read_flag(cpu, Flag::C);
                    cpu->regfile.a -= val - carry;

                    mina_set_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    mina_set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
                    break;
                }

                case Opcode::AND_a_b:
                case Opcode::AND_a_c:
                case Opcode::AND_a_d:
                case Opcode::AND_a_e:
                case Opcode::AND_a_h:
                case Opcode::AND_a_l:
                case Opcode::AND_a_hl_ptr:
                case Opcode::AND_a_a:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    cpu->regfile.a &= val;

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    mina_set_flag(cpu, Flag::H);
                    break;
                }

                case Opcode::AND_a_u8: {
                    cpu->regfile.a &= bus_read_imm8(cpu);

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_flag(cpu, Flag::H);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    break;
                }

                case Opcode::XOR_a_b:
                case Opcode::XOR_a_c:
                case Opcode::XOR_a_d:
                case Opcode::XOR_a_e:
                case Opcode::XOR_a_h:
                case Opcode::XOR_a_l:
                case Opcode::XOR_a_hl_ptr:
                case Opcode::XOR_a_a:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    cpu->regfile.a ^= val;

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    break;
                }

                case Opcode::XOR_a_u8: {
                    cpu->regfile.a ^= bus_read_imm8(cpu);

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    break;
                }

                case Opcode::OR_a_b:
                case Opcode::OR_a_c:
                case Opcode::OR_a_d:
                case Opcode::OR_a_e:
                case Opcode::OR_a_h:
                case Opcode::OR_a_l:
                case Opcode::OR_a_hl_ptr:
                case Opcode::OR_a_a:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    cpu->regfile.a |= val;

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    break;
                }

                case Opcode::OR_a_u8: {
                    u8 val = bus_read_imm8(cpu);
                    cpu->regfile.a |= val;

                    mina_clear_all_flags(cpu->regfile);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    break;
                }

                case Opcode::CP_a_b:
                case Opcode::CP_a_c:
                case Opcode::CP_a_d:
                case Opcode::CP_a_e:
                case Opcode::CP_a_h:
                case Opcode::CP_a_l:
                case Opcode::CP_a_hl_ptr:
                case Opcode::CP_a_a:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);

                    mina_set_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == val);
                    mina_set_or_clear_flag_if(cpu, Flag::C, val > cpu->regfile.a);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        psh_u8_lo(val) > psh_u8_lo(cpu->regfile.a));
                    break;
                }

                case Opcode::CP_a_u8: {
                    u8 val = bus_read_imm8(cpu);

                    mina_set_flag(cpu, Flag::N);
                    mina_set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == val);
                    mina_set_or_clear_flag_if(cpu, Flag::C, val > cpu->regfile.a);
                    mina_set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        psh_u8_lo(val) > psh_u8_lo(cpu->regfile.a));
                    break;
                }

                // TODO(luiz): Implement the remaining instructions.
                case Opcode::RLCA: {
                    break;
                }
                case Opcode::RRCA: {
                    break;
                }
                case Opcode::STOP: {
                    psh_todo_msg("STOP");
                    break;
                }
                case Opcode::RLA: {
                    break;
                }
                case Opcode::RRA: {
                    break;
                }
                case Opcode::DAA: {
                    break;
                }
                case Opcode::CPL: {
                    break;
                }
                case Opcode::SCF: {
                    break;
                }
                case Opcode::CCF: {
                    break;
                }
                case Opcode::RET_nz: {
                    break;
                }
                case Opcode::POP_bc: {
                    break;
                }
                case Opcode::CALL_nz_u16: {
                    break;
                }
                case Opcode::PUSH_bc: {
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
                case Opcode::PREFIX_0xCB: {
                    psh_todo_msg("Prefix 0xCB");
                    break;
                }
                case Opcode::CALL_z_u16: {
                    break;
                }
                case Opcode::CALL_u16: {
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
                case Opcode::CALL_nc_u16: {
                    break;
                }
                case Opcode::PUSH_de: {
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
                case Opcode::CALL_c_u16: {
                    break;
                }
                case Opcode::RST_0x18: {
                    break;
                }
                case Opcode::POP_hl: {
                    break;
                }
                case Opcode::PUSH_hl: {
                    break;
                }
                case Opcode::RST_0x20: {
                    break;
                }
                case Opcode::RST_0x28: {
                    break;
                }
                case Opcode::POP_af: {
                    break;
                }
                case Opcode::DI: {
                    break;
                }
                case Opcode::PUSH_af: {
                    break;
                }
                case Opcode::RST_0x30: {
                    break;
                }
                case Opcode::EI: {
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
