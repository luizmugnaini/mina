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
/// Description: Enumerations of the Game Boy DMG opcodes.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <psh/types.h>

namespace mina::dmg {
    enum struct Opcode : u8 {
        NOP                = 0x00,
        LD_BC_U16          = 0x01,
        LD_BC_PTR_A        = 0x02,
        INC_BC             = 0x03,
        INC_B              = 0x04,
        DEC_B              = 0x05,
        LD_B_U8            = 0x06,
        RLCA               = 0x07,
        LD_U16_PTR_SP      = 0x08,
        ADD_HL_BC          = 0x09,
        LD_A_BC_PTR        = 0x0A,
        DEC_BC             = 0x0B,
        INC_C              = 0x0C,
        DEC_C              = 0x0D,
        LD_C_U8            = 0x0E,
        RRCA               = 0x0F,
        STOP               = 0x10,
        LD_DE_U16          = 0x11,
        LD_DE_PTR_A        = 0x12,
        INC_DE             = 0x13,
        INC_D              = 0x14,
        DEC_D              = 0x15,
        LD_D_U8            = 0x16,
        RLA                = 0x17,
        JR_I8              = 0x18,
        ADD_HL_DE          = 0x19,
        LD_A_DE_PTR        = 0x1A,
        DEC_DE             = 0x1B,
        INC_E              = 0x1C,
        DEC_E              = 0x1D,
        LD_E_U8            = 0x1E,
        RRA                = 0x1F,
        JR_NZ_I8           = 0x20,
        LD_HL_U16          = 0x21,
        LDI_HL_PTR_A       = 0x22,
        INC_HL             = 0x23,
        INC_H              = 0x24,
        DEC_H              = 0x25,
        LD_H_U8            = 0x26,
        DAA                = 0x27,
        JR_Z_I8            = 0x28,
        ADD_HL_HL          = 0x29,
        LDI_A_HL_PTR       = 0x2A,
        DEC_HL             = 0x2B,
        INC_L              = 0x2C,
        DEC_L              = 0x2D,
        LD_L_U8            = 0x2E,
        CPL                = 0x2F,
        JR_NC_I8           = 0x30,
        LD_SP_U16          = 0x31,
        LDD_HL_PTR_A       = 0x32,
        INC_SP             = 0x33,
        INC_HL_PTR         = 0x34,
        DEC_HL_PTR         = 0x35,
        LD_HL_PTR_U8       = 0x36,
        SCF                = 0x37,
        JR_C_I8            = 0x38,
        ADD_HL_SP          = 0x39,
        LDD_A_HL_PTR       = 0x3A,
        DEC_SP             = 0x3B,
        INC_A              = 0x3C,
        DEC_A              = 0x3D,
        LD_A_U8            = 0x3E,
        CCF                = 0x3F,
        LD_B_B             = 0x40,
        LD_B_C             = 0x41,
        LD_B_D             = 0x42,
        LD_B_E             = 0x43,
        LD_B_H             = 0x44,
        LD_B_L             = 0x45,
        LD_B_HL_PTR        = 0x46,
        LD_B_A             = 0x47,
        LD_C_B             = 0x48,
        LD_C_C             = 0x49,
        LD_C_D             = 0x4A,
        LD_C_E             = 0x4B,
        LD_C_H             = 0x4C,
        LD_C_L             = 0x4D,
        LD_C_HL_PTR        = 0x4E,
        LD_C_A             = 0x4F,
        LD_D_B             = 0x50,
        LD_D_C             = 0x51,
        LD_D_D             = 0x52,
        LD_D_E             = 0x53,
        LD_D_H             = 0x54,
        LD_D_L             = 0x55,
        LD_D_HL_PTR        = 0x56,
        LD_D_A             = 0x57,
        LD_E_B             = 0x58,
        LD_E_C             = 0x59,
        LD_E_D             = 0x5A,
        LD_E_E             = 0x5B,
        LD_E_H             = 0x5C,
        LD_E_L             = 0x5D,
        LD_E_HL_PTR        = 0x5E,
        LD_E_A             = 0x5F,
        LD_H_B             = 0x60,
        LD_H_C             = 0x61,
        LD_H_D             = 0x62,
        LD_H_E             = 0x63,
        LD_H_H             = 0x64,
        LD_H_L             = 0x65,
        LD_H_HL_PTR        = 0x66,
        LD_H_A             = 0x67,
        LD_L_B             = 0x68,
        LD_L_C             = 0x69,
        LD_L_D             = 0x6A,
        LD_L_E             = 0x6B,
        LD_L_H             = 0x6C,
        LD_L_L             = 0x6D,
        LD_L_HL_PTR        = 0x6E,
        LD_L_A             = 0x6F,
        LD_HL_PTR_B        = 0x70,
        LD_HL_PTR_C        = 0x71,
        LD_HL_PTR_D        = 0x72,
        LD_HL_PTR_E        = 0x73,
        LD_HL_PTR_H        = 0x74,
        LD_HL_PTR_L        = 0x75,
        HALT               = 0x76,
        LD_HL_PTR_A        = 0x77,
        LD_A_B             = 0x78,
        LD_A_C             = 0x79,
        LD_A_D             = 0x7A,
        LD_A_E             = 0x7B,
        LD_A_H             = 0x7C,
        LD_A_L             = 0x7D,
        LD_A_HL_PTR        = 0x7E,
        LD_A_A             = 0x7F,
        ADD_A_B            = 0x80,
        ADD_A_C            = 0x81,
        ADD_A_D            = 0x82,
        ADD_A_E            = 0x83,
        ADD_A_H            = 0x84,
        ADD_A_L            = 0x85,
        ADD_A_HL_PTR       = 0x86,
        ADD_A_A            = 0x87,
        ADC_A_B            = 0x88,
        ADC_A_C            = 0x89,
        ADC_A_D            = 0x8A,
        ADC_A_E            = 0x8B,
        ADC_A_H            = 0x8C,
        ADC_A_L            = 0x8D,
        ADC_A_HL_PTR       = 0x8E,
        ADC_A_A            = 0x8F,
        SUB_A_B            = 0x90,
        SUB_A_C            = 0x91,
        SUB_A_D            = 0x92,
        SUB_A_E            = 0x93,
        SUB_A_H            = 0x94,
        SUB_A_L            = 0x95,
        SUB_A_HL_PTR       = 0x96,
        SUB_A_A            = 0x97,
        SBC_A_B            = 0x98,
        SBC_A_C            = 0x99,
        SBC_A_D            = 0x9A,
        SBC_A_E            = 0x9B,
        SBC_A_H            = 0x9C,
        SBC_A_L            = 0x9D,
        SBC_A_HL_PTR       = 0x9E,
        SBC_A_A            = 0x9F,
        AND_A_B            = 0xA0,
        AND_A_C            = 0xA1,
        AND_A_D            = 0xA2,
        AND_A_E            = 0xA3,
        AND_A_H            = 0xA4,
        AND_A_L            = 0xA5,
        AND_A_HL_PTR       = 0xA6,
        AND_A_A            = 0xA7,
        XOR_A_B            = 0xA8,
        XOR_A_C            = 0xA9,
        XOR_A_D            = 0xAA,
        XOR_A_E            = 0xAB,
        XOR_A_H            = 0xAC,
        XOR_A_L            = 0xAD,
        XOR_A_HL_PTR       = 0xAE,
        XOR_A_A            = 0xAF,
        OR_A_B             = 0xB0,
        OR_A_C             = 0xB1,
        OR_A_D             = 0xB2,
        OR_A_E             = 0xB3,
        OR_A_H             = 0xB4,
        OR_A_L             = 0xB5,
        OR_A_HL_PTR        = 0xB6,
        OR_A_A             = 0xB7,
        CP_A_B             = 0xB8,
        CP_A_C             = 0xB9,
        CP_A_D             = 0xBA,
        CP_A_E             = 0xBB,
        CP_A_H             = 0xBC,
        CP_A_L             = 0xBD,
        CP_A_HL_PTR        = 0xBE,
        CP_A_A             = 0xBF,
        RET_NZ             = 0xC0,
        POP_BC             = 0xC1,
        JP_NZ_U16          = 0xC2,
        JP_U16             = 0xC3,
        CALL_NZ_U16        = 0xC4,
        PUSH_BC            = 0xC5,
        ADD_A_U8           = 0xC6,
        RST_0x00           = 0xC7,
        RET_Z              = 0xC8,
        RET                = 0xC9,
        JP_Z_U16           = 0xCA,
        PREFIX_0xCB        = 0xCB,
        CALL_Z_U16         = 0xCC,
        CALL_U16           = 0xCD,
        ADC_A_U8           = 0xCE,
        RST_0x08           = 0xCF,
        RET_NC             = 0xD0,
        POP_DE             = 0xD1,
        JP_NC_U16          = 0xD2,
        CALL_NC_U16        = 0xD4,
        PUSH_DE            = 0xD5,
        SUB_A_U8           = 0xD6,
        RST_0x10           = 0xD7,
        RET_C              = 0xD8,
        RETI               = 0xD9,
        JP_C_U16           = 0xDA,
        CALL_C_U16         = 0xDC,
        SBC_A_U8           = 0xDE,
        RST_0x18           = 0xDF,
        LDH_U16_PTR_A      = 0xE0,
        POP_HL             = 0xE1,
        LD_0xFF00_PLUS_C_A = 0xE2,
        PUSH_HL            = 0xE5,
        AND_A_U8           = 0xE6,
        RST_0x20           = 0xE7,
        ADD_SP_I8          = 0xE8,
        JP_HL              = 0xE9,
        LD_U16_PTR_A       = 0xEA,
        XOR_A_U8           = 0xEE,
        RST_0x28           = 0xEF,
        LDH_A_U16_PTR      = 0xF0,
        POP_AF             = 0xF1,
        LD_A_0xFF00_PLUS_C = 0xF2,
        DI                 = 0xF3,
        PUSH_AF            = 0xF5,
        OR_A_U8            = 0xF6,
        RST_0x30           = 0xF7,
        LD_HL_SP_PLUS_I8   = 0xF8,
        LD_SP_HL           = 0xF9,
        LD_A_U16_PTR       = 0xFA,
        EI                 = 0xFB,
        CP_A_U8            = 0xFE,
        RST_0x38           = 0xFF,
    };

    /// The kinds of opcodes that are CB-prefixed.
    ///
    /// Each kind should be matched with the last two bits of the opcode byte.
    enum struct CBPrefixed : u8 {
        ROT = 0b00,
        BIT = 0b01,
        RES = 0b10,
        SET = 0b11,
    };

    /// Kinds of CB-prefixed rotation operations.
    ///
    /// Each rotation kind should be matched with the bits 3..5 of the opcode byte.
    enum struct CBRot : u8 {
        RLC  = 0b000,
        RRC  = 0b001,
        RL   = 0b010,
        RR   = 0b011,
        SLA  = 0b100,
        SRA  = 0b101,
        SWAP = 0b110,
        SRL  = 0b111,
    };
}  // namespace mina::dmg
