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
/// Description: Game Boy DMG CPU implementation.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/cpu/dmg.h>

#include <mina/cpu/dmg_opcodes.h>
#include <psh/assert.h>
#include <psh/bit.h>
#include <psh/intrinsics.h>

// TODO(luiz): implement cycle timing correctness.

namespace mina::dmg {
    namespace {
        /// 8-bit registers in little-endian order.
        enum struct Reg8 : u8 {
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
        enum struct Reg16 : u8 {
            BC = 0x02,
            DE = 0x04,
            HL = 0x06,
            SP = 0x08,
        };

        /// Register file flags.
        enum struct Flag : u8 {
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
        /// Mark whether a flag should be set or not.
        enum struct Cond : u8 {
            NZ,
            Z,
            NC,
            C,
        };

        /// 3-bit unsigned integer constant.
        struct UConst3 {
            u8 val     : 3 = 0b000;
            u8 unused_ : 5;
        };

        //---------------------------------------------------------------------
        // Memory operations.
        //---------------------------------------------------------------------

#define cpu_memory(cpu) reinterpret_cast<u8*>(&cpu->mmap)

#define mmap_write_byte(cpu, dst_addr, val_u8)                                      \
    do {                                                                            \
        *(cpu_memory(cpu) + static_cast<uptr>(dst_addr)) = static_cast<u8>(val_u8); \
    } while (0)

#define mmap_write_word(cpu, dst_addr, val_u16)              \
    do {                                                     \
        u16 addr__                      = (dst_addr);        \
        u16 val__                       = (val_u16);         \
        *(cpu_memory(cpu) + addr__)     = psh_u16_lo(val__); \
        *(cpu_memory(cpu) + addr__ + 1) = psh_u16_hi(val__); \
    } while (0)

        u8 bus_read_byte(CPU* cpu, u16 addr) noexcept {
            u8 const* memory = cpu_memory(cpu);
            cpu->bus_addr    = addr;
            return memory[cpu->bus_addr];
        }

#define bus_read_pc(cpu) bus_read_byte(cpu, cpu->regfile.pc++)

        u8 bus_read_imm8(CPU* cpu) noexcept {
            u8 const* memory = cpu_memory(cpu);
            u8        bt     = memory[cpu->regfile.pc++];
            cpu->bus_addr    = cpu->regfile.pc;
            return bt;
        }

        u16 bus_read_imm16(CPU* cpu) noexcept {
            u8 const* memory = cpu_memory(cpu);
            u8        lo     = memory[cpu->regfile.pc++];
            u8        hi     = memory[cpu->regfile.pc++];
            cpu->bus_addr    = cpu->regfile.pc;
            return psh_u16_from_bytes(hi, lo);
        }

        //---------------------------------------------------------------------
        // Register file operations.
        //---------------------------------------------------------------------

#define cpu_regfile_memory(cpu) reinterpret_cast<u8*>(&cpu->regfile)

#define read_reg16(cpu, reg16)                                   \
    psh_u16_from_bytes(                                          \
        *(cpu_regfile_memory(cpu) + static_cast<u8>(reg16) + 1), \
        *(cpu_regfile_memory(cpu) + static_cast<u8>(reg16)))

#define set_reg16(cpu, reg16, val_u16)                                   \
    do {                                                                 \
        u16 val__    = (val_u16);                                        \
        u8* reg__    = cpu_regfile_memory(cpu) + static_cast<u8>(reg16); \
        *(reg__)     = static_cast<u8>(psh_u16_lo(val__));               \
        *(reg__ + 1) = static_cast<u8>(psh_u16_hi(val__));               \
    } while (0)

        // NOTE(luiz): Things are a bit weird with weird with 8-bit registers due to the convention
        //             of the SM83 CPU opcodes for solving which 8-bit register is being referred.
        //             The ordering is as in `Reg8`, which doesn't reflect the memory layout of the
        //             register file. It's bad but what can I do about it...

        u8 read_reg8(CPU* cpu, Reg8 reg) noexcept {
            u8 val;
            if (reg == Reg8::HL_ptr) {
                val = bus_read_byte(cpu, read_reg16(cpu, Reg16::HL));
            } else if (reg == Reg8::A) {
                val = cpu->regfile.a;
            } else {
                val = *(cpu_regfile_memory(cpu) + static_cast<u8>(reg));
            }
            return val;
        }

        void set_reg8(CPU* cpu, Reg8 reg, u8 val) noexcept {
            if (reg == Reg8::HL_ptr) {
                u8* memory       = cpu_memory(cpu);
                u16 addr         = read_reg16(cpu, Reg16::HL);
                *(memory + addr) = val;
            } else if (reg == Reg8::A) {
                cpu->regfile.a = val;
            } else {
                *(cpu_regfile_memory(cpu) + static_cast<u8>(reg)) = val;
            }
        }

        //---------------------------------------------------------------------
        // Register flag operations.
        //---------------------------------------------------------------------

#define read_flag(cpu, flag)  psh_bit_at(cpu->regfile.f, static_cast<u8>(flag))
#define set_flag(cpu, flag)   psh_bit_set(cpu->regfile.f, static_cast<u8>(flag))
#define clear_flag(cpu, flag) psh_bit_clear(cpu->regfile.f, static_cast<u8>(flag))
#define set_or_clear_flag_if(cpu, flag, cond) \
    psh_bit_set_or_clear_if(cpu->regfile.f, static_cast<u8>(flag), cond)

#define clear_all_flags(cpu)   \
    do {                       \
        cpu->regfile.f = 0x00; \
    } while (0)

        bool read_condition_flag(CPU* cpu, Cond cc) {
            bool res;
            switch (cc) {
                case Cond::NZ: res = (read_flag(cpu, Flag::Z) == 0); break;
                case Cond::Z:  res = (read_flag(cpu, Flag::Z) != 0); break;
                case Cond::NC: res = (read_flag(cpu, Flag::C) == 0); break;
                case Cond::C:  res = (read_flag(cpu, Flag::C) != 0); break;
            }
            return res;
        }

        //---------------------------------------------------------------------
        // Decode and execute instructions.
        //---------------------------------------------------------------------

        // Decode and execute 0xCB-prefixed instructions.
        void cb_dexec(CPU* cpu) {
            // Byte next to the `0xCB` prefix code.
            u8 data = bus_read_pc(cpu);

            // The 8-bit register that the instruction will act upon.
            Reg8 reg = static_cast<Reg8>(psh_bits_at(data, 0, 3));

            CBPrefixed operation_kind = static_cast<CBPrefixed>(psh_bits_at(data, 6, 2));
            switch (operation_kind) {
                case CBPrefixed::ROT: {
                    // Decode the rotation operation that should be execution.
                    CBRot rot_op = static_cast<CBRot>(psh_bits_at(data, 3, 3));

                    switch (rot_op) {
                        case CBRot::RLC: {
                            u8 res                  = read_reg8(cpu, reg);
                            u8 reg_last_bit_was_set = (psh_bit_at(res, 7) != 0);

                            res = psh_int_rotl(res, 1);
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_last_bit_was_set);
                            break;
                        }
                        case CBRot::RRC: {
                            u8 res                   = read_reg8(cpu, reg);
                            u8 reg_first_bit_was_set = (psh_bit_at(res, 0) != 0);

                            res = psh_int_rotr(res, 1);
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }
                        case CBRot::RL: {
                            u8   res                  = read_reg8(cpu, reg);
                            bool reg_last_bit_was_set = (psh_bit_at(res, 7) != 0);

                            res = psh_int_rotl(res, 1);
                            if (read_flag(cpu, Flag::C) != 0) {
                                psh_bit_set(res, 0);  // Set the first bit.
                            }
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_last_bit_was_set);
                            break;
                        }
                        case CBRot::RR: {
                            u8   res                   = read_reg8(cpu, reg);
                            bool reg_first_bit_was_set = (psh_bit_at(res, 0) != 0);

                            res = psh_int_rotr(res, 1);
                            if (read_flag(cpu, Flag::C) != 0) {
                                psh_bit_set(res, 7);  // Set the last bit.
                            }
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }
                        case CBRot::SLA: {
                            u8   res                  = read_reg8(cpu, reg);
                            bool reg_last_bit_was_set = (psh_bit_at(res, 7) != 0);

                            res <<= 1;
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_last_bit_was_set);
                            break;
                        }
                        case CBRot::SRA: {
                            u8   res                   = read_reg8(cpu, reg);
                            bool reg_first_bit_was_set = (psh_bit_at(res, 0) != 0);

                            res >>= 1;
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }
                        case CBRot::SWAP: {
                            u8 res = read_reg8(cpu, reg);
                            res    = psh_u8_from_nibbles(psh_u8_lo(res), psh_u8_hi(res));
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            break;
                        }
                        case CBRot::SRL: {
                            u8   res                   = read_reg8(cpu, reg);
                            bool reg_first_bit_was_set = (psh_bit_at(res, 0) != 0);

                            res >>= 1;
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }
                    }
                }
                case CBPrefixed::BIT: {
                    u8 bit_pos = psh_bits_at(data, 3, 5);
                    psh_todo_msg("continue BIT op");
                    break;
                }
                case CBPrefixed::RES:
                case CBPrefixed::SET: break;
            }
        }

        void dexec(CPU* cpu, u8 data) noexcept {
            auto opcode = static_cast<Opcode>(data);
            u8   y      = psh_bits_at(data, 3, 3);
            u8   p      = psh_bits_at(y, 1, 2);
            u8   z      = psh_bits_at(data, 0, 3);

            switch (opcode) {
                case Opcode::Nop: {
                    break;
                }

                case Opcode::HALT: {  // NOTE(luiz): Only exception, load [HL] [HL] causes a halt.
                                      // WARN(luiz): Don't change halt position with respect to LD
                                      // R8 R8
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
                    set_reg16(cpu, static_cast<Reg16>(p), bus_read_imm16(cpu));
                    break;
                }
                case Opcode::LD_sp_hl: {
                    set_reg16(cpu, Reg16::SP, read_reg16(cpu, Reg16::HL));
                    break;
                }

                case Opcode::LD_hl_sp_plus_i8: {
                    i8  offset = static_cast<i8>(bus_read_imm8(cpu));
                    u16 sp     = read_reg16(cpu, Reg16::SP);
                    u32 res    = static_cast<u32>(sp + offset);
                    set_reg16(cpu, Reg16::SP, static_cast<u16>(res));
                    set_reg16(cpu, Reg16::HL, static_cast<u16>(res));

                    clear_flag(cpu, Flag::Z);
                    clear_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0xFFFF);
                    set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        static_cast<u16>(psh_u16_lo(sp) + offset) > 0xFF);
                    break;
                }

                case Opcode::LD_bc_ptr_a: {
                    mmap_write_byte(cpu, read_reg16(cpu, Reg16::BC), cpu->regfile.a);
                    break;
                }

                case Opcode::LD_u16_ptr_sp: {
                    mmap_write_word(cpu, bus_read_imm16(cpu), read_reg16(cpu, Reg16::SP));
                    break;
                }
                case Opcode::LD_u16_ptr_a: {
                    mmap_write_byte(cpu, bus_read_imm16(cpu), cpu->regfile.a);
                    break;
                }
                case Opcode::LD_a_u16_ptr: {
                    cpu->regfile.a = bus_read_byte(cpu, bus_read_imm16(cpu));
                    break;
                }

                case Opcode::LDH_u16_ptr_a: {
                    u16 addr = bus_read_imm16(cpu);
                    if (0xFF00 <= addr) {
                        mmap_write_byte(cpu, addr, cpu->regfile.a);
                    }
                    break;
                }
                case Opcode::LDH_a_u16_ptr: {
                    u16 addr = bus_read_imm16(cpu);
                    if (0xFF00 <= addr) {
                        u8 const* memory = cpu_memory(cpu);
                        cpu->regfile.a   = *(memory + addr);
                    }
                    break;
                }

                case Opcode::LD_a_bc_ptr: {
                    cpu->regfile.a = bus_read_byte(cpu, read_reg16(cpu, Reg16::BC));
                    break;
                }
                case Opcode::LD_a_de_ptr: {
                    cpu->regfile.a = bus_read_byte(cpu, read_reg16(cpu, Reg16::DE));
                    break;
                }
                case Opcode::LD_de_ptr_a: {
                    mmap_write_byte(cpu, read_reg16(cpu, Reg16::DE), cpu->regfile.a);
                    break;
                }

                case Opcode::LDI_hl_ptr_a: {
                    u16 hl = read_reg16(cpu, Reg16::HL);
                    mmap_write_byte(cpu, hl, cpu->regfile.a);
                    set_reg16(cpu, Reg16::HL, hl + 1);
                    break;
                }
                case Opcode::LDD_hl_ptr_a: {
                    u16 hl = read_reg16(cpu, Reg16::HL);
                    mmap_write_byte(cpu, hl, cpu->regfile.a);
                    set_reg16(cpu, Reg16::HL, hl - 1);
                    break;
                }
                case Opcode::LDI_a_hl_ptr: {
                    u16 hl         = read_reg16(cpu, Reg16::HL);
                    cpu->regfile.a = bus_read_byte(cpu, hl);
                    set_reg16(cpu, Reg16::HL, hl + 1);
                    break;
                }
                case Opcode::LDD_a_hl_ptr: {
                    u16 hl         = read_reg16(cpu, Reg16::HL);
                    cpu->regfile.a = bus_read_byte(cpu, hl);
                    set_reg16(cpu, Reg16::HL, hl - 1);
                    break;
                }

                case Opcode::LD_0xFF00_plus_c_a: {
                    mmap_write_byte(cpu, 0xFF00 + cpu->regfile.c, cpu->regfile.a);
                    break;
                }
                case Opcode::LD_a_0xFF00_plus_c: {
                    cpu->regfile.a = *(cpu_memory(cpu) + 0xFF00 + cpu->regfile.c);
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
                    cpu->regfile.pc = read_reg16(cpu, Reg16::HL);
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

                    clear_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, prev_val + 1 == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) + 1 > 0x0F);
                    break;
                }

                case Opcode::INC_bc:
                case Opcode::INC_de:
                case Opcode::INC_hl:
                case Opcode::INC_sp: {
                    Reg16 reg = static_cast<Reg16>(p);
                    set_reg16(cpu, reg, read_reg16(cpu, reg) + 1);
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

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, prev_val - 1 == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) - 1 > 0x0F);
                    break;
                }

                case Opcode::DEC_bc:
                case Opcode::DEC_de:
                case Opcode::DEC_hl:
                case Opcode::DEC_sp: {
                    Reg16 reg = static_cast<Reg16>(p);
                    set_reg16(cpu, reg, read_reg16(cpu, reg) - 1);
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

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    set_or_clear_flag_if(cpu, Flag::H, (psh_u8_lo(acc) + psh_u8_lo(val)) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    break;
                }

                case Opcode::ADD_a_u8: {
                    u8  val        = bus_read_imm8(cpu);
                    u8  acc        = cpu->regfile.a;
                    u16 res        = static_cast<u16>(acc + val);
                    cpu->regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    set_or_clear_flag_if(cpu, Flag::H, (psh_u8_lo(acc) + psh_u8_lo(val)) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    break;
                }

                case Opcode::ADD_sp_i8: {
                    i8  offset = static_cast<i8>(bus_read_imm8(cpu));
                    u16 sp     = read_reg16(cpu, Reg16::SP);
                    u32 res    = static_cast<u32>(sp + offset);
                    set_reg16(cpu, Reg16::SP, static_cast<u16>(res));

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0xFFFF);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u16_lo(sp) + offset > 0x00FF);
                    break;
                }

                case Opcode::ADD_hl_bc:
                case Opcode::ADD_hl_de:
                case Opcode::ADD_hl_hl:
                case Opcode::ADD_hl_sp: {
                    Reg16 reg = static_cast<Reg16>(p);
                    u16   val = read_reg16(cpu, reg);
                    u16   hl  = read_reg16(cpu, Reg16::HL);
                    set_reg16(cpu, Reg16::HL, static_cast<u16>(hl + val));

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, hl + val > 0xFFFF);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u16_lo(hl) + psh_u16_lo(val) > 0x00FF);
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
                    u8   carry     = read_flag(cpu, Flag::C);
                    u16  res       = static_cast<u16>(acc + val + carry);
                    cpu->regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    break;
                }

                case Opcode::ADC_a_u8: {
                    u8  val        = bus_read_imm8(cpu);
                    u8  acc        = cpu->regfile.a;
                    u8  carry      = read_flag(cpu, Flag::C);
                    u16 res        = static_cast<u16>(acc + val + carry);
                    cpu->regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
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

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val > acc);
                    break;
                }

                case Opcode::SUB_a_u8: {
                    u8 val = bus_read_imm8(cpu);
                    u8 acc = cpu->regfile.a;
                    cpu->regfile.a -= val;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val > acc);
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
                    u8   carry = read_flag(cpu, Flag::C);
                    cpu->regfile.a -= val - carry;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
                    break;
                }

                case Opcode::SBC_a_u8: {
                    u8 val   = bus_read_imm8(cpu);
                    u8 acc   = cpu->regfile.a;
                    u8 carry = read_flag(cpu, Flag::C);
                    cpu->regfile.a -= val - carry;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
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

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    set_flag(cpu, Flag::H);
                    break;
                }

                case Opcode::AND_a_u8: {
                    cpu->regfile.a &= bus_read_imm8(cpu);

                    clear_all_flags(cpu);
                    set_flag(cpu, Flag::H);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
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

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    break;
                }

                case Opcode::XOR_a_u8: {
                    cpu->regfile.a ^= bus_read_imm8(cpu);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
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

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
                    break;
                }

                case Opcode::OR_a_u8: {
                    u8 val = bus_read_imm8(cpu);
                    cpu->regfile.a |= val;

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == 0);
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

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == val);
                    set_or_clear_flag_if(cpu, Flag::C, val > cpu->regfile.a);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(cpu->regfile.a));
                    break;
                }

                case Opcode::CP_a_u8: {
                    u8 val = bus_read_imm8(cpu);

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu->regfile.a == val);
                    set_or_clear_flag_if(cpu, Flag::C, val > cpu->regfile.a);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(cpu->regfile.a));
                    break;
                }

                // TODO(luiz): Implement the remaining instructions.
                case Opcode::RLCA: {
                    bool will_carry = (psh_bit_at(cpu->regfile.a, 7) != 0);
                    cpu->regfile.a  = psh_int_rotl(cpu->regfile.a, 1);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, will_carry);
                    break;
                }
                case Opcode::RRCA: {
                    bool will_carry = (psh_bit_at(cpu->regfile.a, 0) != 0);
                    cpu->regfile.a  = psh_int_rotr(cpu->regfile.a, 1);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, will_carry);
                    break;
                }

                case Opcode::RLA: {
                    bool acc_last_bit_was_set = (psh_bit_at(cpu->regfile.a, 7) != 0);

                    cpu->regfile.a = psh_int_rotl(cpu->regfile.a, 1);
                    if (read_flag(cpu, Flag::C) != 0) {
                        psh_bit_set(cpu->regfile.a, 0);  // Set the first bit.
                    }

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, acc_last_bit_was_set);
                    break;
                }
                case Opcode::RRA: {
                    bool acc_first_bit_was_set = (psh_bit_at(cpu->regfile.a, 0) != 0);

                    cpu->regfile.a = psh_int_rotr(cpu->regfile.a, 1);
                    if (read_flag(cpu, Flag::C) != 0) {
                        psh_bit_set(cpu->regfile.a, 7);  // Set the last bit.
                    }

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, acc_first_bit_was_set);
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

                case Opcode::PREFIX_0xCB: {
                    cb_dexec(cpu);
                    break;
                }

                case Opcode::STOP: {
                    psh_todo_msg("STOP");
                    break;
                }
            }
        }
    }  // namespace

    void CPU::run_cycle() noexcept {
        u8 data = bus_read_pc(this);
        dexec(this, data);
    }
}  // namespace mina::dmg
