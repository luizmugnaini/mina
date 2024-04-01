/// Game Boy's CPU.
///
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/base.h>
#include <mina/memory_map.h>

namespace mina {
    struct ControlUnit {};

    /// 8-bit based Arithmetic Logic Unit (ALU).
    ///
    /// This is a unit is responsible for performing calculations and outputting the results to
    /// the register file or the CPU bus.
    struct ArithmeticLogicUnit {
        u8 port_a = 0;  ///< First ALU port.
        u8 port_b = 0;  ///< Second ALU port.
    };

    /// Increment/Decrement Unit (IDU).
    ///
    /// Responsible for increment and decrement operations of the 16-bit address bus value. Outputs
    /// its result to the register file in a 16-bit register or an 8-bit register pair.
    struct IncDecUnit {
        u16 port = 0;
    };

    /// Original Game Boy CPU.
    ///
    /// The DMG is an SoC containing a Sharp SM83 CPU, which is based on the Zilog Z80 and Intel
    /// 8080.
    struct DMG {
        ControlUnit         ctrl{};
        HwRegisterBank      reg{};
        ArithmeticLogicUnit alu{};
        IncDecUnit          idu{};
    };
}  // namespace mina
