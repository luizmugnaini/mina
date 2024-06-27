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
/// Description: Game Boy Sharp SM83 CPU implementation.
///
/// Highly recommended reads for the decoding of opcodes:
/// - [RGBDS, CPU opcode reference](https://rgbds.gbdev.io/docs/v0.7.0/gbz80.7)
/// - [Decoding Game Boy Z80 Opcodes](
///   https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html).
///
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/sm83.h>

#include <mina/sm83_opcodes.h>
#include <psh/assert.h>
#include <psh/bit.h>
#include <psh/intrinsics.h>

// TODO(luiz): implement cycle timing correctness.

namespace mina {
    // -----------------------------------------------------------------------------
    // - Implementation details -
    // -----------------------------------------------------------------------------

    namespace {
        // -----------------------------------------------------------------------------
        // - Registers -
        // -----------------------------------------------------------------------------

        /// 8-bit registers in little-endian order.
        ///
        /// The value of each 8-bit register corresponds to its offset in bytes from the start of
        /// the register file. Note however that `HL_PTR` isn't an actual register and therefore
        /// doesn't have an offset, so we put -1 and treat it as a separate case when dealing with
        /// 8-bit registers.
        enum struct Reg8 {
            B      = 3,
            C      = 2,
            D      = 5,
            E      = 4,
            H      = 7,
            L      = 6,
            HL_PTR = -1,
            A      = 1,
        };

        /// 16-bit registers.
        ///
        /// The value of each 16-bit register corresponds to its offset in bytes from the start of
        /// the register file.
        enum struct Reg16 : u8 {
            BC = 2,
            DE = 4,
            HL = 6,
            SP = 8,
        };

        /// Alternate 16-bit register set.
        ///
        /// The value of each 16-bit register corresponds to its offset in bytes from the start of
        /// the register file.
        enum struct AltReg16 : u8 {
            BC = 2,
            DE = 4,
            HL = 6,
            AF = 0,
        };

        /// Register file flags.
        ///
        /// The value associated to each flag corresponds to its offset in bits from the start of
        /// the flag register.
        enum struct Flag : u8 {
            /// The carry flag. Set in the following occasions:
            ///     * 8-bit addition is higher than 0xFF.
            ///     * 16-bit addition is higher than 0xFFFF.
            ///     * Result of subtraction or comparison is negative.
            ///     * If a shift operation shifts out a 0b1 valued bit.
            C = 4,
            H = 5,  ///< Indicates carry for the high nibble.
            N = 6,  ///< If the last operation was a subtraction.
            Z = 7,  ///< If the last operation result was zero.
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

        //---------------------------------------------------------------------
        // Memory operations.
        //---------------------------------------------------------------------

/// Get the CPU memory map in bytes
#define cpu_memory(cpu) reinterpret_cast<u8*>(&cpu.mmap)

/// Write a byte to a given address in the CPU memory map.
#define mmap_write_byte(cpu, dst_addr, val_u8)                                      \
    do {                                                                            \
        *(cpu_memory(cpu) + static_cast<uptr>(dst_addr)) = static_cast<u8>(val_u8); \
    } while (0)

/// Write a word (16-bit) to a given address in the CPU memory map.
#define mmap_write_word(cpu, dst_addr, val_u16)                  \
    do {                                                         \
        *(cpu_memory(cpu) + dst_addr)     = psh_u16_lo(val_u16); \
        *(cpu_memory(cpu) + dst_addr + 1) = psh_u16_hi(val_u16); \
    } while (0)

        /// Read the byte at a given address in the CPU memory map.
        u8 bus_read_byte(SM83& cpu, u16 addr) noexcept {
            u8 const* memory = cpu_memory(cpu);
            cpu.bus_addr     = addr;
            return memory[cpu.bus_addr];
        }

/// Read the byte at the program counter and advance.
#define bus_read_pc(cpu) bus_read_byte(cpu, cpu.regfile.pc++)

        /// Read the immediate 8-bit value in the program counter, and advance 1 byte.
        u8 bus_read_imm8(SM83& cpu) noexcept {
            u8 const* memory = cpu_memory(cpu);
            u8        bt     = memory[cpu.regfile.pc++];
            cpu.bus_addr     = cpu.regfile.pc;
            return bt;
        }

        /// Read the immediate 16-bit value in the program counter (little-endian), and advance 2
        /// bytes.
        u16 bus_read_imm16(SM83& cpu) noexcept {
            u8 const* memory = cpu_memory(cpu);
            u8        lo     = memory[cpu.regfile.pc++];
            u8        hi     = memory[cpu.regfile.pc++];
            cpu.bus_addr     = cpu.regfile.pc;
            return psh_u16_from_bytes(hi, lo);
        }

        //---------------------------------------------------------------------
        // Register file operations.
        //---------------------------------------------------------------------

/// Get the register file in bytes.
#define cpu_regfile_memory(cpu) reinterpret_cast<u8*>(&cpu.regfile)

/// Read the value of a 16-bit register.
#define read_reg16(cpu, reg16)                                   \
    psh_u16_from_bytes(                                          \
        *(cpu_regfile_memory(cpu) + static_cast<u8>(reg16) + 1), \
        *(cpu_regfile_memory(cpu) + static_cast<u8>(reg16)))

/// Set the value of a 16-bit register.
#define set_reg16(cpu, reg16, val_u16)                                \
    do {                                                              \
        u8* reg__ = cpu_regfile_memory(cpu) + static_cast<u8>(reg16); \
        reg__[0]  = static_cast<u8>(psh_u16_lo(val_u16));             \
        reg__[1]  = static_cast<u8>(psh_u16_hi(val_u16));             \
    } while (0)

        // NOTE(luiz): Things are a bit weird with weird with 8-bit registers due to the convention
        //             of the SM83 CPU opcodes for solving which 8-bit register is being referred.
        //             The ordering is as in `Reg8`, which doesn't reflect the memory layout of the
        //             register file. It's bad but what can I do about it...

        /// Read the value of an 8-bit register.
        u8 read_reg8(SM83& cpu, Reg8 reg) noexcept {
            u8 val;
            if (reg == Reg8::HL_PTR) {
                val = bus_read_byte(cpu, read_reg16(cpu, Reg16::HL));
            } else {
                val = *(cpu_regfile_memory(cpu) + static_cast<u8>(reg));
            }
            return val;
        }

        /// Set the value of an 8-bit register.
        void set_reg8(SM83& cpu, Reg8 reg, u8 val) noexcept {
            if (reg == Reg8::HL_PTR) {
                u8* memory       = cpu_memory(cpu);
                u16 addr         = read_reg16(cpu, Reg16::HL);
                *(memory + addr) = val;
            } else {
                *(cpu_regfile_memory(cpu) + static_cast<u8>(reg)) = val;
            }
        }

        //---------------------------------------------------------------------
        // Register flag operations.
        //---------------------------------------------------------------------

#define test_flag(cpu, flag)  static_cast<bool>(read_flag(cpu, flag))
#define read_flag(cpu, flag)  psh_bit_at(cpu.regfile.f, static_cast<u8>(flag))
#define set_flag(cpu, flag)   psh_bit_set(cpu.regfile.f, static_cast<u8>(flag))
#define clear_flag(cpu, flag) psh_bit_clear(cpu.regfile.f, static_cast<u8>(flag))
#define set_or_clear_flag_if(cpu, flag, cond) \
    psh_bit_set_or_clear_if(cpu.regfile.f, static_cast<u8>(flag), cond)
#define clear_all_flags(cpu)  \
    do {                      \
        cpu.regfile.f = 0x00; \
    } while (0)

        /// Get the conditional result of a flag being set or not.
        bool read_condition_flag(SM83& cpu, Cond cc) {
            bool res;
            switch (cc) {
                case Cond::NZ: res = (read_flag(cpu, Flag::Z) == 0); break;
                case Cond::Z:  res = (read_flag(cpu, Flag::Z) == 1); break;
                case Cond::NC: res = (read_flag(cpu, Flag::C) == 0); break;
                case Cond::C:  res = (read_flag(cpu, Flag::C) == 1); break;
            }
            return res;
        }

        // -----------------------------------------------------------------------------
        // - Register decoding. -
        // -----------------------------------------------------------------------------

        /// 8-bit register decoding mapping.
        ///
        /// Maps the encoded 8-bit register into its corresponding register.
        constexpr Reg8 DECODE_REG8[8] =
            {Reg8::B, Reg8::C, Reg8::D, Reg8::E, Reg8::H, Reg8::L, Reg8::HL_PTR, Reg8::A};

        /// 16-bit register decoding mapping.
        ///
        /// As part of the decoding process, in order to obtain the referenced 16-bit register, we
        /// have to look at the bits 4 and 5 of the opcode --- denoted in `decode_and_execute` by
        /// the variable `p`. This variable can assume values between 0 and 3, therefore we have to
        /// map those into the corresponding `Reg16` enum value.
        constexpr Reg16 DECODE_REG16[4] = {
            Reg16::BC,
            Reg16::DE,
            Reg16::HL,
            Reg16::SP,
        };

        /// Alternate 16-bit register decoding mapping, used for instructions such as push and pop.
        constexpr AltReg16 DECODE_ALT_REG16[4] = {
            AltReg16::BC,
            AltReg16::DE,
            AltReg16::HL,
            AltReg16::AF,
        };

        //---------------------------------------------------------------------
        // Instruction decoding and execution.
        //---------------------------------------------------------------------

        /// Decode and execute 0xCB-prefixed instructions.
        inline void cb_decode_and_execute(SM83& cpu, u8 data) {
            // The 8-bit register that the instruction will act upon.
            u8   y   = psh_bits_at(data, 0, 3);
            Reg8 reg = DECODE_REG8[y];

            CBPrefixed operation_kind = static_cast<CBPrefixed>(psh_bits_at(data, 6, 8));
            switch (operation_kind) {
                // Rotate/shift a register value or memory.
                case CBPrefixed::ROT: {
                    // Decode the rotation operation that should be execution.
                    CBRot rot_op = static_cast<CBRot>(psh_bit_at(data, 3));

                    switch (rot_op) {
                        // Rotate left with carry.
                        case CBRot::RLC: {
                            u8 res                  = read_reg8(cpu, reg);
                            u8 reg_last_bit_was_set = psh_test_bit_at(res, 7);

                            res = psh_int_rotl(res, 1);
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_last_bit_was_set);
                            break;
                        }

                        // Rotate right with carry.
                        case CBRot::RRC: {
                            u8 res                   = read_reg8(cpu, reg);
                            u8 reg_first_bit_was_set = psh_test_bit_at(res, 0);

                            res = psh_int_rotr(res, 1);
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }

                        // Rotate left through the carry flag.
                        case CBRot::RL: {
                            u8   res                  = read_reg8(cpu, reg);
                            bool reg_last_bit_was_set = psh_test_bit_at(res, 7);

                            res = psh_int_rotl(res, 1);
                            if (test_flag(cpu, Flag::C)) {
                                psh_bit_set(res, 0);  // Set the first bit.
                            }
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_last_bit_was_set);
                            break;
                        }

                        // Rotate right through the carry flag.
                        case CBRot::RR: {
                            u8   res                   = read_reg8(cpu, reg);
                            bool reg_first_bit_was_set = psh_test_bit_at(res, 0);

                            res = psh_int_rotr(res, 1);
                            if (test_flag(cpu, Flag::C)) {
                                psh_bit_set(res, 7);  // Set the last bit.
                            }
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }

                        // Shift left arithmetically, with carry.
                        case CBRot::SLA: {
                            u8   res                  = read_reg8(cpu, reg);
                            bool reg_last_bit_was_set = psh_test_bit_at(res, 7);

                            res <<= 1;
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_last_bit_was_set);
                            break;
                        }

                        // Shift right arithmetically, with carry.
                        case CBRot::SRA: {
                            u8   res                   = read_reg8(cpu, reg);
                            bool reg_first_bit_was_set = psh_test_bit_at(res, 0);

                            res >>= 1;
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }

                        // Swap the value of the low and high nibbles.
                        case CBRot::SWAP: {
                            u8 res = read_reg8(cpu, reg);
                            res    = psh_u8_from_nibbles(psh_u8_lo(res), psh_u8_hi(res));
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            break;
                        }

                        // Shift right logically, with carry.
                        case CBRot::SRL: {
                            u8   res                   = read_reg8(cpu, reg);
                            bool reg_first_bit_was_set = psh_test_bit_at(res, 0);

                            res >>= 1;
                            set_reg8(cpu, reg, res);

                            clear_all_flags(cpu);
                            set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                            set_or_clear_flag_if(cpu, Flag::C, reg_first_bit_was_set);
                            break;
                        }
                    }
                }

                // Test if a given bit of the value of a register is set or clear.
                case CBPrefixed::BIT: {
                    u8   test_bit_pos = psh_bits_at(data, 3, 5);
                    bool is_set       = psh_test_bit_at(read_reg8(cpu, reg), test_bit_pos);

                    set_flag(cpu, Flag::H);
                    clear_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::Z, !is_set);
                    break;
                }

                // Clear a bit of a register to 0.
                case CBPrefixed::RES: {
                    u8 clear_bit_pos = psh_bits_at(data, 3, 5);
                    u8 res           = read_reg8(cpu, reg) & psh_not_bit(u8, clear_bit_pos);
                    set_reg8(cpu, reg, res);
                    break;
                }

                // Set a bit of a register to 1.
                case CBPrefixed::SET: {
                    u8 set_bit_pos = psh_bits_at(data, 3, 5);
                    u8 res         = read_reg8(cpu, reg) | psh_bit(u8, set_bit_pos);
                    set_reg8(cpu, reg, res);
                    break;
                }
            }
        }

        /// Decode and execute instructions
        void decode_and_execute(SM83& cpu, u8 data) noexcept {
            Opcode op = static_cast<Opcode>(data);

            // Strip information out of the opcode.
            u8 y = psh_bits_at(data, 3, 3);
            u8 p = psh_bits_at(y, 1, 2);
            u8 z = psh_bits_at(data, 0, 3);

            switch (op) {
                // Do nothing.
                case Opcode::NOP:  break;

                // LD R8 R8.
                //
                // Exception: Halt is caused by a LD [HL] [HL] instruction. For this reason we put
                //            it as the first case, in order to break earlier from the switch. For
                //            this reason, do not change the position of `case Opcode::HALT` with
                //            respect to the remaining LD R8 R8 instructions.
                case Opcode::HALT: {
                    psh_todo_msg("HALT");
                    break;
                }
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
                    set_reg8(cpu, DECODE_REG8[y], read_reg8(cpu, DECODE_REG8[z]));
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
                    set_reg8(cpu, DECODE_REG8[y], bus_read_imm8(cpu));
                    break;
                }

                // Load the unsigned immediate 16-bit value to the given 16-bit register.
                case Opcode::LD_BC_U16:
                case Opcode::LD_DE_U16:
                case Opcode::LD_HL_U16:
                case Opcode::LD_SP_U16: {
                    set_reg16(cpu, DECODE_REG16[p], bus_read_imm16(cpu));
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

                    clear_flag(cpu, Flag::N);
                    clear_flag(cpu, Flag::Z);
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
                    Reg8 reg      = DECODE_REG8[y];
                    u8   prev_val = read_reg8(cpu, reg);
                    set_reg8(cpu, reg, prev_val + 1);

                    clear_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) + 1 > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, prev_val + 1 == 0);
                    break;
                }

                // Increment a given 16-bit register.
                case Opcode::INC_BC:
                case Opcode::INC_DE:
                case Opcode::INC_HL:
                case Opcode::INC_SP: {
                    Reg16 reg = DECODE_REG16[p];
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
                    Reg8 reg      = DECODE_REG8[y];
                    u8   prev_val = read_reg8(cpu, reg);
                    set_reg8(cpu, reg, prev_val - 1);

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(prev_val) - 1 > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, prev_val - 1 == 0);
                    break;
                }

                // Decremement a given 16-bit register.
                case Opcode::DEC_BC:
                case Opcode::DEC_DE:
                case Opcode::DEC_HL:
                case Opcode::DEC_SP: {
                    Reg16 reg = DECODE_REG16[p];
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
                    Reg8 reg      = DECODE_REG8[z];
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
                    Reg16 reg = DECODE_REG16[p];
                    u16   val = read_reg16(cpu, reg);
                    u16   hl  = read_reg16(cpu, Reg16::HL);
                    set_reg16(cpu, Reg16::HL, static_cast<u16>(hl + val));

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, hl + val > 0xFFFF);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u16_lo(hl) + psh_u16_lo(val) > 0x00FF);
                    break;
                }

                // Add to the accumulator register, considering the carry flag, the value of an
                // 8-bit register.
                case Opcode::ADC_A_B:
                case Opcode::ADC_A_C:
                case Opcode::ADC_A_D:
                case Opcode::ADC_A_E:
                case Opcode::ADC_A_H:
                case Opcode::ADC_A_L:
                case Opcode::ADC_A_HL_PTR:
                case Opcode::ADC_A_A:      {
                    Reg8 reg      = DECODE_REG8[z];
                    u8   val      = read_reg8(cpu, reg);
                    u8   acc      = cpu.regfile.a;
                    u8   carry    = read_flag(cpu, Flag::C);
                    u16  res      = static_cast<u16>(acc + val + carry);
                    cpu.regfile.a = static_cast<u8>(res);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
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
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x00FF);
                    set_or_clear_flag_if(
                        cpu,
                        Flag::H,
                        (psh_u8_lo(acc) + psh_u8_lo(val) + carry) > 0x0F);
                    set_or_clear_flag_if(cpu, Flag::Z, res == 0);
                    break;
                }

                // Subtract from the accumulator register the value of an 8-bit register.
                case Opcode::SUB_A_B:
                case Opcode::SUB_A_C:
                case Opcode::SUB_A_D:
                case Opcode::SUB_A_E:
                case Opcode::SUB_A_H:
                case Opcode::SUB_A_L:
                case Opcode::SUB_A_HL_PTR:
                case Opcode::SUB_A_A:      {
                    Reg8 reg = DECODE_REG8[z];
                    u8   val = read_reg8(cpu, reg);
                    u8   acc = cpu.regfile.a;
                    cpu.regfile.a -= val;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::C, val > acc);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                // Subtract from the accumulator register the immediate 8-bit value.
                case Opcode::SUB_A_U8: {
                    u8 val = bus_read_imm8(cpu);
                    u8 acc = cpu.regfile.a;
                    cpu.regfile.a -= val;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::C, val > acc);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                // Subtract from the accumulator register, considering the carry flag, the value
                // of an 8-bit register.
                case Opcode::SBC_A_B:
                case Opcode::SBC_A_C:
                case Opcode::SBC_A_D:
                case Opcode::SBC_A_E:
                case Opcode::SBC_A_H:
                case Opcode::SBC_A_L:
                case Opcode::SBC_A_HL_PTR:
                case Opcode::SBC_A_A:      {
                    Reg8 reg   = DECODE_REG8[z];
                    u8   val   = read_reg8(cpu, reg);
                    u8   acc   = cpu.regfile.a;
                    u8   carry = read_flag(cpu, Flag::C);
                    cpu.regfile.a -= val - carry;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
                    break;
                }

                /// Subtract from the accumulator register, considering the carry flag, the
                /// immediate 8-bit value.
                case Opcode::SBC_A_U8: {
                    u8 val   = bus_read_imm8(cpu);
                    u8 acc   = cpu.regfile.a;
                    u8 carry = read_flag(cpu, Flag::C);
                    cpu.regfile.a -= val - carry;

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::C, val + carry > acc);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) + carry > psh_u8_lo(acc));
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
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
                    Reg8 reg = DECODE_REG8[z];
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
                    Reg8 reg = DECODE_REG8[z];
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
                    Reg8 reg = DECODE_REG8[z];
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
                    Reg8 reg = DECODE_REG8[z];
                    u8   val = read_reg8(cpu, reg);

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::C, val > cpu.regfile.a);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(cpu.regfile.a));
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == val);
                    break;
                }

                case Opcode::CP_A_U8: {
                    u8 val = bus_read_imm8(cpu);

                    set_flag(cpu, Flag::N);
                    set_or_clear_flag_if(cpu, Flag::C, val > cpu.regfile.a);
                    set_or_clear_flag_if(cpu, Flag::H, psh_u8_lo(val) > psh_u8_lo(cpu.regfile.a));
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == val);
                    break;
                }

                case Opcode::RLCA: {
                    bool will_carry = psh_test_bit_at(cpu.regfile.a, 7);
                    cpu.regfile.a   = psh_int_rotl(cpu.regfile.a, 1);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, will_carry);
                    break;
                }
                case Opcode::RRCA: {
                    bool will_carry = psh_test_bit_at(cpu.regfile.a, 0);
                    cpu.regfile.a   = psh_int_rotr(cpu.regfile.a, 1);

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, will_carry);
                    break;
                }

                case Opcode::RLA: {
                    bool acc_last_bit_was_set = psh_test_bit_at(cpu.regfile.a, 7);

                    cpu.regfile.a = psh_int_rotl(cpu.regfile.a, 1);
                    if (test_flag(cpu, Flag::C)) {
                        psh_bit_set(cpu.regfile.a, 0);  // Set the first bit.
                    }

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, acc_last_bit_was_set);
                    break;
                }
                case Opcode::RRA: {
                    bool acc_first_bit_was_set = psh_test_bit_at(cpu.regfile.a, 0);

                    cpu.regfile.a = psh_int_rotr(cpu.regfile.a, 1);
                    if (test_flag(cpu, Flag::C)) {
                        psh_bit_set(cpu.regfile.a, 7);  // Set the last bit.
                    }

                    clear_all_flags(cpu);
                    set_or_clear_flag_if(cpu, Flag::C, acc_first_bit_was_set);
                    break;
                }

                // Decimal adjust the accumulator.
                //
                // Adjust the accumulator to get a correct binary coded decimal (BCD)
                // representation. This is probably the strangest instruction, so I'll explain in
                // more detail below.
                //
                // NOTE(luiz): BCD is completely stupid and inefficient. TL;DR BCD the decimal
                //             representation into the binary/hex world. For instance, 32 is
                //             0b0010_0000 but in BCD representation it is 0b0011_0010 where the
                //             high nibble 0b0011 is 3 and the low nibble 0b0010 is 2 (forming 32,
                //             get it?).
                //
                // Non-subtraction adjustment (flag N clear):
                //
                // - Low nibble: in BCD a low nibble can have value at most 0x09. Due to this, if
                //   the last operation had a half carry or the low nibble is greater than 0x09 then
                //   we have to add 0x06 to the value in order to transfer the sparing bits from the
                //   low nibble to the high nibble.
                //
                //   Essentially, this is the same as mapping 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, and 0x0F
                //   to the hexadecimals 0x00, 0x10, ..., and 0x50. Note that in doing this we may
                //   get a high nibble which doesn't fit in the BCD format, so we'll have to adjust
                //   it next.
                //
                // - High nibble: since the high nibble can be at most 0x90, either if the last
                //   operation had a carry or the high nibble is greater than 0x09, we have to
                //   adjust the hexadecimal representation in order to fit the BCD. In order to
                //   achieve this we add 0x60 to the value, which essentially maps the hexadecimals
                //   0xA0, ..., 0xF0 to the values 0x00, ..., 0x05.
                //
                // Subtraction adjustment (flag N set):
                //
                // - Low nibble: if a half carry occurred in the last subtraction, we have to shift
                //   our low nibble back to the range of 0x00 and 0x09. In order to do so, 0x0F
                //   should be mapped to 0x09. This gives us a clue that we may subtract 0x06 from
                //   our value. This gives us a mapping from 0x0A, ..., 0x0F to 0x04, ..., 0x09.
                //
                // - High nibble: using an analogous reasoning as before, if a carry occurred we
                //   subtract 0x60 from our value in order to adjust its high nibble.
                case Opcode::DAA: {
                    constexpr u8 LOW_NIBBLE_ADJUST = 0x06;
                    constexpr u8 HI_NIBBLE_ADJUST  = 0x60;

                    u16 res = static_cast<u16>(cpu.regfile.a);
                    if (test_flag(cpu, Flag::N)) {
                        if (test_flag(cpu, Flag::H)) {
                            res -= LOW_NIBBLE_ADJUST;
                        }

                        if (test_flag(cpu, Flag::C)) {
                            res -= HI_NIBBLE_ADJUST;
                        }
                    } else {
                        if (test_flag(cpu, Flag::H) || (res & 0x0F) > 0x09) {
                            res += LOW_NIBBLE_ADJUST;
                        }

                        if (test_flag(cpu, Flag::C) || (res & 0xFFF0) > 0x90) {
                            res += HI_NIBBLE_ADJUST;
                        }
                    }
                    cpu.regfile.a = psh_u16_lo(res);

                    clear_flag(cpu, Flag::H);
                    set_or_clear_flag_if(cpu, Flag::C, res > 0x99);
                    set_or_clear_flag_if(cpu, Flag::Z, cpu.regfile.a == 0);
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
                    cpu.regfile.f ^= psh_bit(u8, static_cast<u8>(Flag::C));

                    clear_flag(cpu, Flag::H);
                    clear_flag(cpu, Flag::N);
                    break;
                }

                // Write the 16-bit register value to the stack.
                case Opcode::PUSH_BC:
                case Opcode::PUSH_DE:
                case Opcode::PUSH_HL:
                case Opcode::PUSH_AF: {
                    u16 val = read_reg16(cpu, DECODE_ALT_REG16[p]);
                    cpu.regfile.sp -= 2;
                    mmap_write_word(cpu, cpu.regfile.sp, val);
                    break;
                }

                case Opcode::RET_NZ: {
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
                case Opcode::RET_NC: {
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

                case Opcode::POP_BC: {
                    psh_todo();
                    break;
                }
                case Opcode::POP_DE: {
                    psh_todo();
                    break;
                }
                case Opcode::POP_HL: {
                    psh_todo();
                    break;
                }
                case Opcode::POP_AF: {
                    psh_todo();
                    break;
                }

                case Opcode::CALL_NZ_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_Z_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_NC_U16: {
                    psh_todo();
                    break;
                }
                case Opcode::CALL_C_U16: {
                    psh_todo();
                    break;
                }

                case Opcode::CALL_U16: {
                    psh_todo();
                    break;
                }

                case Opcode::RST_0x00: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x08: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x10: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x18: {
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
                case Opcode::RST_0x30: {
                    psh_todo();
                    break;
                }
                case Opcode::RST_0x38: {
                    psh_todo();
                    break;
                }

                case Opcode::DI: {
                    psh_todo();
                    break;
                }
                case Opcode::EI: {
                    psh_todo();
                    break;
                }

                // Decode and execute the 0xCB-prefixed opcode.
                case Opcode::PREFIX_0xCB: {
                    cb_decode_and_execute(cpu, bus_read_pc(cpu));
                    break;
                }

                case Opcode::STOP: {
                    psh_todo_msg("STOP");
                    break;
                }

                // Received an ill opcode.
                default: psh_unreachable();
            }
        }
    }  // namespace

    // -----------------------------------------------------------------------------
    // - Implementation of the CPU's public interface. -
    // -----------------------------------------------------------------------------

    void sm83_run_cycle(SM83& cpu) noexcept {
        u8 data = bus_read_pc(cpu);

        psh_debug(opcode_to_string(static_cast<Opcode>(data)));

        decode_and_execute(cpu, data);
    }
}  // namespace mina
