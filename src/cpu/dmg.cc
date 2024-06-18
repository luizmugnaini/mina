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
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/cpu/dmg.h>

#include <mina/cpu/dmg_opcodes.h>
#include <psh/assert.h>
#include <psh/bit.h>
#include <psh/intrinsics.h>

// TODO(luiz): implement cycle timing correctness.

namespace mina {
    namespace {
        /// 8-bit registers in little-endian order.
        enum struct Reg8 : u8 {
            B      = 0x00,
            C      = 0x01,
            D      = 0x02,
            E      = 0x03,
            H      = 0x04,
            L      = 0x05,
            HL_PTR = 0x06,
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

#define cpu_memory(cpu) reinterpret_cast<u8*>(&cpu.mmap)

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

        u8 bus_read_byte(CPU& cpu, u16 addr) noexcept {
            u8 const* memory = cpu_memory(cpu);
            cpu.bus_addr     = addr;
            return memory[cpu.bus_addr];
        }

#define bus_read_pc(cpu) bus_read_byte(cpu, cpu.regfile.pc++)

        u8 bus_read_imm8(CPU& cpu) noexcept {
            u8 const* memory = cpu_memory(cpu);
            u8        bt     = memory[cpu.regfile.pc++];
            cpu.bus_addr     = cpu.regfile.pc;
            return bt;
        }

        u16 bus_read_imm16(CPU& cpu) noexcept {
            u8 const* memory = cpu_memory(cpu);
            u8        lo     = memory[cpu.regfile.pc++];
            u8        hi     = memory[cpu.regfile.pc++];
            cpu.bus_addr     = cpu.regfile.pc;
            return psh_u16_from_bytes(hi, lo);
        }

        //---------------------------------------------------------------------
        // Register file operations.
        //---------------------------------------------------------------------

#define cpu_regfile_memory(cpu) reinterpret_cast<u8*>(&cpu.regfile)

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

        u8 read_reg8(CPU& cpu, Reg8 reg) noexcept {
            u8 val;
            if (reg == Reg8::HL_PTR) {
                val = bus_read_byte(cpu, read_reg16(cpu, Reg16::HL));
            } else if (reg == Reg8::A) {
                val = cpu.regfile.a;
            } else {
                val = *(cpu_regfile_memory(cpu) + static_cast<u8>(reg));
            }
            return val;
        }

        void set_reg8(CPU& cpu, Reg8 reg, u8 val) noexcept {
            if (reg == Reg8::HL_PTR) {
                u8* memory       = cpu_memory(cpu);
                u16 addr         = read_reg16(cpu, Reg16::HL);
                *(memory + addr) = val;
            } else if (reg == Reg8::A) {
                cpu.regfile.a = val;
            } else {
                *(cpu_regfile_memory(cpu) + static_cast<u8>(reg)) = val;
            }
        }

        //---------------------------------------------------------------------
        // Register flag operations.
        //---------------------------------------------------------------------

#define read_flag(cpu, flag)  psh_bit_at(cpu.regfile.f, static_cast<u8>(flag))
#define set_flag(cpu, flag)   psh_bit_set(cpu.regfile.f, static_cast<u8>(flag))
#define clear_flag(cpu, flag) psh_bit_clear(cpu.regfile.f, static_cast<u8>(flag))
#define set_or_clear_flag_if(cpu, flag, cond) \
    psh_bit_set_or_clear_if(cpu.regfile.f, static_cast<u8>(flag), cond)

#define clear_all_flags(cpu)  \
    do {                      \
        cpu.regfile.f = 0x00; \
    } while (0)

        bool read_condition_flag(CPU& cpu, Cond cc) {
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
        void cb_dexec(CPU& cpu) {
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
                // TODO(luiz): implement RES and SET.
                case CBPrefixed::RES: psh_todo_msg("CB RES");
                case CBPrefixed::SET: psh_todo_msg("CB SET");
            }
        }

        void dexec(CPU& cpu, u8 data) noexcept {
            Opcode op = static_cast<Opcode>(data);

            // Strip information out of the opcode byte.
            u8 y = psh_bits_at(data, 3, 3);
            u8 p = psh_bits_at(y, 1, 2);
            u8 z = psh_bits_at(data, 0, 3);

            switch (op) {
                // Do nothing.
                case Opcode::NOP:         break;

                // Halt is caused by a LD [HL] [HL] instruction.
                //
                // NOTE(luiz): Please, do not change the the position of this instruction case with
                //             respect to the below LD R8 R8 instructions as it'll be the only
                //             exception to the pattern.
                case Opcode::HALT:        psh_todo_msg("HALT");
                case Opcode::LD_B_B:
                case Opcode::LD_B_C:
                case Opcode::LD_B_D:
                case Opcode::LD_B_E:
                case Opcode::LD_B_H:
                case Opcode::LD_B_L:
                case Opcode::LD_B_HL_PTR:
                case Opcode::LD_B_A:
                case Opcode::LD_C_B:
                case Opcode::LD_C_C:
                case Opcode::LD_C_D:
                case Opcode::LD_C_E:
                case Opcode::LD_C_H:
                case Opcode::LD_C_L:
                case Opcode::LD_C_HL_PTR:
                case Opcode::LD_C_A:
                case Opcode::LD_D_B:
                case Opcode::LD_D_C:
                case Opcode::LD_D_D:
                case Opcode::LD_D_E:
                case Opcode::LD_D_H:
                case Opcode::LD_D_L:
                case Opcode::LD_D_HL_PTR:
                case Opcode::LD_D_A:
                case Opcode::LD_E_B:
                case Opcode::LD_E_C:
                case Opcode::LD_E_D:
                case Opcode::LD_E_E:
                case Opcode::LD_E_H:
                case Opcode::LD_E_L:
                case Opcode::LD_E_HL_PTR:
                case Opcode::LD_E_A:
                case Opcode::LD_H_B:
                case Opcode::LD_H_C:
                case Opcode::LD_H_D:
                case Opcode::LD_H_E:
                case Opcode::LD_H_H:
                case Opcode::LD_H_L:
                case Opcode::LD_H_HL_PTR:
                case Opcode::LD_H_A:
                case Opcode::LD_L_B:
                case Opcode::LD_L_C:
                case Opcode::LD_L_D:
                case Opcode::LD_L_E:
                case Opcode::LD_L_H:
                case Opcode::LD_L_L:
                case Opcode::LD_L_HL_PTR:
                case Opcode::LD_L_A:
                case Opcode::LD_HL_PTR_B:
                case Opcode::LD_HL_PTR_C:
                case Opcode::LD_HL_PTR_D:
                case Opcode::LD_HL_PTR_E:
                case Opcode::LD_HL_PTR_H:
                case Opcode::LD_HL_PTR_L:
                case Opcode::LD_HL_PTR_A:
                case Opcode::LD_A_B:
                case Opcode::LD_A_C:
                case Opcode::LD_A_D:
                case Opcode::LD_A_E:
                case Opcode::LD_A_H:
                case Opcode::LD_A_L:
                case Opcode::LD_A_HL_PTR:
                case Opcode::LD_A_A:      {
                    set_reg8(cpu, static_cast<Reg8>(y), read_reg8(cpu, static_cast<Reg8>(z)));
                    break;
                }

                // Load the unsigned immediate 8-bit value to the given 8-bit register.
                case Opcode::LD_B_U8:
                case Opcode::LD_C_U8:
                case Opcode::LD_D_U8:
                case Opcode::LD_E_U8:
                case Opcode::LD_H_U8:
                case Opcode::LD_L_U8:
                case Opcode::LD_HL_PTR_U8:
                case Opcode::LD_A_U8:      {
                    set_reg8(cpu, static_cast<Reg8>(y), bus_read_imm8(cpu));
                    break;
                }

                // Load the unsigned immediate 16-bit value to the given 16-bit register.
                case Opcode::LD_BC_U16:
                case Opcode::LD_DE_U16:
                case Opcode::LD_HL_U16:
                case Opcode::LD_SP_U16: {
                    set_reg16(cpu, static_cast<Reg16>(p), bus_read_imm16(cpu));
                    break;
                }

                // Load the value of the HL register to the stack pointer.
                case Opcode::LD_SP_HL: {
                    set_reg16(cpu, Reg16::SP, read_reg16(cpu, Reg16::HL));
                    break;
                }

                // Load the result of the addition of the stack pointer and the signed immediate
                // 8-bit value to the HL register.
                case Opcode::LD_HL_SP_PLUS_I8: {
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

                // Load the value of the accumulator register to the byte whose address is given by
                // the 16-bit register BC.
                case Opcode::LD_BC_PTR_A: {
                    mmap_write_byte(cpu, read_reg16(cpu, Reg16::BC), cpu.regfile.a);
                    break;
                }

                // Load the value of the stack pointer to the byte whose address is given by the
                // unsigned immediate 16-bit value.
                case Opcode::LD_U16_PTR_SP: {
                    mmap_write_word(cpu, bus_read_imm16(cpu), read_reg16(cpu, Reg16::SP));
                    break;
                }

                // Load the value of the accumulator register to the byte whose address is given by
                // the unsigned immediate 16-bit value.
                case Opcode::LD_U16_PTR_A: {
                    mmap_write_byte(cpu, bus_read_imm16(cpu), cpu.regfile.a);
                    break;
                }

                // Load the value of the byte whose address is given by the unsigned immediate
                // 16-bit value to the accumulator register.
                case Opcode::LD_A_U16_PTR: {
                    cpu.regfile.a = bus_read_byte(cpu, bus_read_imm16(cpu));
                    break;
                }

                case Opcode::LDH_U16_PTR_A: {
                    u16 addr = bus_read_imm16(cpu);
                    if (0xFF00 <= addr) {
                        mmap_write_byte(cpu, addr, cpu.regfile.a);
                    }
                    break;
                }

                case Opcode::LDH_A_U16_PTR: {
                    u16 addr = bus_read_imm16(cpu);
                    if (0xFF00 <= addr) {
                        u8 const* memory = cpu_memory(cpu);
                        cpu.regfile.a    = *(memory + addr);
                    }
                    break;
                }

                case Opcode::LD_A_BC_PTR: {
                    cpu.regfile.a = bus_read_byte(cpu, read_reg16(cpu, Reg16::BC));
                    break;
                }
                case Opcode::LD_A_DE_PTR: {
                    cpu.regfile.a = bus_read_byte(cpu, read_reg16(cpu, Reg16::DE));
                    break;
                }

                case Opcode::LD_DE_PTR_A: {
                    mmap_write_byte(cpu, read_reg16(cpu, Reg16::DE), cpu.regfile.a);
                    break;
                }

                case Opcode::LDI_HL_PTR_A: {
                    u16 hl = read_reg16(cpu, Reg16::HL);
                    mmap_write_byte(cpu, hl, cpu.regfile.a);
                    set_reg16(cpu, Reg16::HL, hl + 1);
                    break;
                }
                case Opcode::LDD_HL_PTR_A: {
                    u16 hl = read_reg16(cpu, Reg16::HL);
                    mmap_write_byte(cpu, hl, cpu.regfile.a);
                    set_reg16(cpu, Reg16::HL, hl - 1);
                    break;
                }
                case Opcode::LDI_A_HL_PTR: {
                    u16 hl        = read_reg16(cpu, Reg16::HL);
                    cpu.regfile.a = bus_read_byte(cpu, hl);
                    set_reg16(cpu, Reg16::HL, hl + 1);
                    break;
                }
                case Opcode::LDD_A_HL_PTR: {
                    u16 hl        = read_reg16(cpu, Reg16::HL);
                    cpu.regfile.a = bus_read_byte(cpu, hl);
                    set_reg16(cpu, Reg16::HL, hl - 1);
                    break;
                }

                case Opcode::LD_0xFF00_PLUS_C_A: {
                    mmap_write_byte(cpu, 0xFF00 + cpu.regfile.c, cpu.regfile.a);
                    break;
                }
                case Opcode::LD_A_0xFF00_PLUS_C: {
                    cpu.regfile.a = *(cpu_memory(cpu) + 0xFF00 + cpu.regfile.c);
                    break;
                }

                // Jump the program counter relative to its current position by a signed immediate
                // 8-bit value.
                case Opcode::JR_I8: {
                    i8 rel_addr = static_cast<i8>(bus_read_imm16(cpu));
                    cpu.regfile.pc += rel_addr;
                    break;
                }

                // Conditionally jump the program counter relative to its current position by a
                // signed immediate 8-bit value.
                case Opcode::JR_NZ_I8:
                case Opcode::JR_Z_I8:
                case Opcode::JR_NC_I8:
                case Opcode::JR_C_I8:  {
                    Cond cc = static_cast<Cond>(y - 4);
                    if (read_condition_flag(cpu, cc)) {
                        i8 rel_addr = static_cast<i8>(bus_read_imm16(cpu));
                        cpu.regfile.pc += rel_addr;
                    }
                    break;
                }

                // Jump the program counter to the address given by the 16-bit register HL.
                case Opcode::JP_HL: {
                    cpu.regfile.pc = read_reg16(cpu, Reg16::HL);
                    break;
                }

                // Jump the program counter to the address given by an unsigned immediate 8-bit
                // value.
                case Opcode::JP_U16: {
                    cpu.regfile.pc = bus_read_imm16(cpu);
                    break;
                }

                // Conditionally jump the program counter to the address given by an unsigned
                // immediate 16-bit value.
                case Opcode::JP_NZ_U16:
                case Opcode::JP_Z_U16:
                case Opcode::JP_NC_U16:
                case Opcode::JP_C_U16:  {
                    Cond cc = static_cast<Cond>(y);
                    if (read_condition_flag(cpu, cc)) {
                        cpu.regfile.pc = bus_read_imm16(cpu);
                    }
                    break;
                }

                // Increment a given 8-bit register.
                case Opcode::INC_B:
                case Opcode::INC_C:
                case Opcode::INC_D:
                case Opcode::INC_E:
                case Opcode::INC_H:
                case Opcode::INC_L:
                case Opcode::INC_HL_PTR:
                case Opcode::INC_A:      {
                    Reg8 reg      = static_cast<Reg8>(y);
                    u8   prev_val = read_reg8(cpu, reg);
                    set_reg8(cpu, reg, prev_val + 1);

                    clear_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, prev_val + 1 == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) + 1 > 0x0F);
                    break;
                }

                // Increment a given 16-bit register.
                case Opcode::INC_BC:
                case Opcode::INC_DE:
                case Opcode::INC_HL:
                case Opcode::INC_SP: {
                    Reg16 reg = static_cast<Reg16>(p);
                    set_reg16(cpu, reg, read_reg16(cpu, reg) + 1);
                    break;
                }

                // Decrement a given 8-bit register.
                case Opcode::DEC_B:
                case Opcode::DEC_C:
                case Opcode::DEC_D:
                case Opcode::DEC_E:
                case Opcode::DEC_H:
                case Opcode::DEC_L:
                case Opcode::DEC_HL_PTR:
                case Opcode::DEC_A:      {
                    Reg8 reg      = static_cast<Reg8>(y);
                    u8   prev_val = read_reg8(cpu, reg);
                    set_reg8(cpu, reg, prev_val - 1);

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, prev_val - 1 == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) - 1 > 0x0F);
                    break;
                }

                // Decremement a given 16-bit register.
                case Opcode::DEC_BC:
                case Opcode::DEC_DE:
                case Opcode::DEC_HL:
                case Opcode::DEC_SP: {
                    Reg16 reg = static_cast<Reg16>(p);
                    set_reg16(cpu, reg, read_reg16(cpu, reg) - 1);
                    break;
                }

                // Add the value contained in an 8-bit register to the accumulator register.
                case Opcode::ADD_A_B:
                case Opcode::ADD_A_C:
                case Opcode::ADD_A_D:
                case Opcode::ADD_A_E:
                case Opcode::ADD_A_H:
                case Opcode::ADD_A_L:
                case Opcode::ADD_A_HL_PTR:
                case Opcode::ADD_A_A:      {
                    Reg8 reg      = static_cast<Reg8>(z);
                    u8   val      = read_reg8(cpu, reg);
                    u8   acc      = cpu.regfile.a;
                    u16  res      = static_cast<u16>(acc + val);
                    cpu.regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    set_or_clear_flag_if(cpu, Flag::H, (psh_u8_lo(acc) + psh_u8_lo(val)) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    break;
                }

                // Add an unsigned immediate 8-bit value to the accumulator register.
                case Opcode::ADD_A_U8: {
                    u8  val       = bus_read_imm8(cpu);
                    u8  acc       = cpu.regfile.a;
                    u16 res       = static_cast<u16>(acc + val);
                    cpu.regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    set_or_clear_flag_if(cpu, Flag::H, (psh_u8_lo(acc) + psh_u8_lo(val)) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    break;
                }

                // Add a signed 8-bit immediate value from the stack pointer.
                case Opcode::ADD_SP_I8: {
                    i8  offset = static_cast<i8>(bus_read_imm8(cpu));
                    u16 sp     = read_reg16(cpu, Reg16::SP);
                    u32 res    = static_cast<u32>(sp + offset);
                    set_reg16(cpu, Reg16::SP, static_cast<u16>(res));

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0xFFFF);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u16_lo(sp) + offset > 0x00FF);
                    break;
                }

                // Add the value of a 16-bit register to the HL register.
                case Opcode::ADD_HL_BC:
                case Opcode::ADD_HL_DE:
                case Opcode::ADD_HL_HL:
                case Opcode::ADD_HL_SP: {
                    Reg16 reg = static_cast<Reg16>(p);
                    u16   val = read_reg16(cpu, reg);
                    u16   hl  = read_reg16(cpu, Reg16::HL);
                    set_reg16(cpu, Reg16::HL, static_cast<u16>(hl + val));

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, hl + val > 0xFFFF);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u16_lo(hl) + psh_u16_lo(val) > 0x00FF);
                    break;
                }

                // Add, considering the carry flag, the value contained in a given 8-bit register to
                // the accumulator register.
                case Opcode::ADC_A_B:
                case Opcode::ADC_A_C:
                case Opcode::ADC_A_D:
                case Opcode::ADC_A_E:
                case Opcode::ADC_A_H:
                case Opcode::ADC_A_L:
                case Opcode::ADC_A_HL_PTR:
                case Opcode::ADC_A_A:      {
                    Reg8 reg      = static_cast<Reg8>(z);
                    u8   val      = read_reg8(cpu, reg);
                    u8   acc      = cpu.regfile.a;
                    u8   carry    = read_flag(cpu, Flag::C);
                    u16  res      = static_cast<u16>(acc + val + carry);
                    cpu.regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    break;
                }

                // Add an unsigned immediate 8-bit value to the accumulator register.
                case Opcode::ADC_A_U8: {
                    u8  val       = bus_read_imm8(cpu);
                    u8  acc       = cpu.regfile.a;
                    u8  carry     = read_flag(cpu, Flag::C);
                    u16 res       = static_cast<u16>(acc + val + carry);
                    cpu.regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    break;
                }

                // Subtract the value contained in a given 8-bit register from the accumulator
                // register.
                case Opcode::SUB_A_B:
                case Opcode::SUB_A_C:
                case Opcode::SUB_A_D:
                case Opcode::SUB_A_E:
                case Opcode::SUB_A_H:
                case Opcode::SUB_A_L:
                case Opcode::SUB_A_HL_PTR:
                case Opcode::SUB_A_A:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    u8   acc = cpu.regfile.a;
                    cpu.regfile.a -= val;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val > acc);
                    break;
                }

                // Subtract from the accumulator register the immediate 8-bit value.
                case Opcode::SUB_A_U8: {
                    u8 val = bus_read_imm8(cpu);
                    u8 acc = cpu.regfile.a;
                    cpu.regfile.a -= val;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val > acc);
                    break;
                }

                // Subtract, considering the carry flag, the value contained in a given 8-bit
                // register from the accumulator register.
                case Opcode::SBC_A_B:
                case Opcode::SBC_A_C:
                case Opcode::SBC_A_D:
                case Opcode::SBC_A_E:
                case Opcode::SBC_A_H:
                case Opcode::SBC_A_L:
                case Opcode::SBC_A_HL_PTR:
                case Opcode::SBC_A_A:      {
                    Reg8 reg   = static_cast<Reg8>(z);
                    u8   val   = read_reg8(cpu, reg);
                    u8   acc   = cpu.regfile.a;
                    u8   carry = read_flag(cpu, Flag::C);
                    cpu.regfile.a -= val - carry;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
                    break;
                }

                case Opcode::SBC_A_U8: {
                    u8 val   = bus_read_imm8(cpu);
                    u8 acc   = cpu.regfile.a;
                    u8 carry = read_flag(cpu, Flag::C);
                    cpu.regfile.a -= val - carry;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
                    break;
                }

                case Opcode::AND_A_B:
                case Opcode::AND_A_C:
                case Opcode::AND_A_D:
                case Opcode::AND_A_E:
                case Opcode::AND_A_H:
                case Opcode::AND_A_L:
                case Opcode::AND_A_HL_PTR:
                case Opcode::AND_A_A:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    cpu.regfile.a &= val;

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    set_flag(cpu, Flag::H);
                    break;
                }

                case Opcode::AND_A_U8: {
                    cpu.regfile.a &= bus_read_imm8(cpu);

                    clear_all_flags(cpu);
                    set_flag(cpu, Flag::H);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                case Opcode::XOR_A_B:
                case Opcode::XOR_A_C:
                case Opcode::XOR_A_D:
                case Opcode::XOR_A_E:
                case Opcode::XOR_A_H:
                case Opcode::XOR_A_L:
                case Opcode::XOR_A_HL_PTR:
                case Opcode::XOR_A_A:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    cpu.regfile.a ^= val;

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                case Opcode::XOR_A_U8: {
                    cpu.regfile.a ^= bus_read_imm8(cpu);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                case Opcode::OR_A_B:
                case Opcode::OR_A_C:
                case Opcode::OR_A_D:
                case Opcode::OR_A_E:
                case Opcode::OR_A_H:
                case Opcode::OR_A_L:
                case Opcode::OR_A_HL_PTR:
                case Opcode::OR_A_A:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);
                    cpu.regfile.a |= val;

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                case Opcode::OR_A_U8: {
                    u8 val = bus_read_imm8(cpu);
                    cpu.regfile.a |= val;

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                case Opcode::CP_A_B:
                case Opcode::CP_A_C:
                case Opcode::CP_A_D:
                case Opcode::CP_A_E:
                case Opcode::CP_A_H:
                case Opcode::CP_A_L:
                case Opcode::CP_A_HL_PTR:
                case Opcode::CP_A_A:      {
                    Reg8 reg = static_cast<Reg8>(z);
                    u8   val = read_reg8(cpu, reg);

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == val);
                    set_or_clear_flag_if(cpu, Flag::C, val > cpu.regfile.a);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(cpu.regfile.a));
                    break;
                }

                case Opcode::CP_A_U8: {
                    u8 val = bus_read_imm8(cpu);

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == val);
                    set_or_clear_flag_if(cpu, Flag::C, val > cpu.regfile.a);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(cpu.regfile.a));
                    break;
                }

                // TODO(luiz): Implement the remaining instructions.
                case Opcode::RLCA: {
                    bool will_carry = (psh_bit_at(cpu.regfile.a, 7) != 0);
                    cpu.regfile.a   = psh_int_rotl(cpu.regfile.a, 1);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, will_carry);
                    break;
                }
                case Opcode::RRCA: {
                    bool will_carry = (psh_bit_at(cpu.regfile.a, 0) != 0);
                    cpu.regfile.a   = psh_int_rotr(cpu.regfile.a, 1);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, will_carry);
                    break;
                }

                case Opcode::RLA: {
                    bool acc_last_bit_was_set = (psh_bit_at(cpu.regfile.a, 7) != 0);

                    cpu.regfile.a = psh_int_rotl(cpu.regfile.a, 1);
                    if (read_flag(cpu, Flag::C) != 0) {
                        psh_bit_set(cpu.regfile.a, 0);  // Set the first bit.
                    }

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, acc_last_bit_was_set);
                    break;
                }
                case Opcode::RRA: {
                    bool acc_first_bit_was_set = (psh_bit_at(cpu.regfile.a, 0) != 0);

                    cpu.regfile.a = psh_int_rotr(cpu.regfile.a, 1);
                    if (read_flag(cpu, Flag::C) != 0) {
                        psh_bit_set(cpu.regfile.a, 7);  // Set the last bit.
                    }

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, acc_first_bit_was_set);
                    break;
                }

                // Decimal adjust the accumulator.
                //
                // Adjust the accumulator to get a correct binary coded decimal (BCD)
                // representation.
                //
                // NOTE(luiz): BCD is completely stupid, it basically takes the decimal
                //             representation into the binary world. For instance, 32 is 0b0010_0000
                //             but in BCD representation it is 0b0011_0010 where the high nibble
                //             0b0011 is 3 and the low nibble 0b0010 is 2 (forming 32, get it?).
                case Opcode::DAA: {
                    clear_flag(cpu, Flag::H);
                    break;
                }

                // Write the complement of the accumulator to the accumulator register.
                case Opcode::CPL: {
                    cpu.regfile.a = ~cpu.regfile.a;

                    set_flag(cpu, Flag::H);
                    set_flag(cpu, Flag::N);
                    break;
                }

                // Set the carry flag
                case Opcode::SCF: {
                    set_flag(cpu, Flag::C);
                    clear_flag(cpu, Flag::H);
                    clear_flag(cpu, Flag::N);
                    break;
                }

                // Invert the carry flag.
                case Opcode::CCF: {
                    break;
                }

                case Opcode::RET_NZ: {
                    psh_todo();
                    break;
                }
                case Opcode::POP_BC: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_NZ_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::PUSH_BC: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x00: {
                    psh_todo();
                    break;
                }
                case Opcode::RET_Z: {
                    psh_todo();
                    break;
                }
                case Opcode::RET: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_Z_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x08: {
                    psh_todo();
                    break;
                }
                case Opcode::RET_NC: {
                    psh_todo();
                    break;
                }
                case Opcode::POP_DE: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_NC_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::PUSH_DE: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x10: {
                    psh_todo();
                    break;
                }
                case Opcode::RET_C: {
                    psh_todo();
                    break;
                }
                case Opcode::RETI: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_C_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x18: {
                    psh_todo();
                    break;
                }
                case Opcode::POP_HL: {
                    psh_todo();
                    break;
                }
                case Opcode::PUSH_HL: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x20: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x28: {
                    psh_todo();
                    break;
                }
                case Opcode::POP_AF: {
                    psh_todo();
                    break;
                }
                case Opcode::DI: {
                    psh_todo();
                    break;
                }
                case Opcode::PUSH_AF: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x30: {
                    psh_todo();
                    break;
                }
                case Opcode::EI: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x38: {
                    psh_todo();
                    break;
                }

                // Decode and execute the 0xCB-prefixed opcode.
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

    void run_cpu_cycle(CPU& cpu) noexcept {
        u8 data = bus_read_pc(cpu);
        dexec(cpu, data);
    }
}  // namespace mina
