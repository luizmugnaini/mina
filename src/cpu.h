#pragma once

#include "base.h"

struct ControlUnit {};

/// Register file.
///
/// Responsible for holding the state of the CPU registers.
struct RegisterFile {
    u16 pc;   ///< Program counter.
    u16 sp;   ///< Stack pointer.
    u8  acc;  ///< Accumulator register.
    u8  f;    ///< Flag register.
    u8  bc;   ///< General purpose BC 8-bit register pair.
    u8  de;   ///< General purpose DE 8-bit register pair.
    u8  hl;   ///< General purpose HL 8-bit register pair.
    u8  ir;   ///< Instruction register.
    u8  ie;   ///< Interrupt enable.
};

/// 8-bit based Arithmetic Logic Unit (ALU).
///
/// This is a unit is responsible for performing calculations and outputting the results to
/// the register file or the CPU bus.
struct ArithmeticLogicUnit {
    u8 port_a;  ///< First ALU port.
    u8 port_b;  ///< Second ALU port.
};

/// Increment/Decrement Unit (IDU).
///
/// Responsible for increment and decrement operations of the 16-bit address bus value. Outputs its
/// result to the register file in a 16-bit register or an 8-bit register pair.
struct IncDecUnit {
    u16 port;
};

/// Original Game Boy CPU.
///
/// The DMG is an SoC containing a Sharp SM83 CPU, which is based on the Zilog Z80 and Intel
/// 8080.
struct DotMatrixGame {
    ControlUnit         cu;
    RegisterFile        reg;
    ArithmeticLogicUnit alu;
    IncDecUnit          idu;
};
