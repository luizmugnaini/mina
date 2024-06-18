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
/// Description: Implementation of the Game Boy's CPU memory map layer.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/memory_map.h>
#include <cstring>

namespace mina {
    void transfer_fixed_rom_bank(Cartridge const& cart, MemoryMap& mmap) noexcept {
        constexpr usize BANK_SIZE = FxROMBank::RANGE.size();
        std::memset(&mmap.fx_rom, 0xFF, BANK_SIZE);

        usize copy_size = psh_min(BANK_SIZE, cart.content.data.size);
        std::memcpy(&mmap.fx_rom, cart.content.data.buf, copy_size);
    }

    psh::Buffer<char, 11> extract_cart_title(MemoryMap& mmap) noexcept {
        constexpr usize TITLE_FIRST_IDX =
            FxROMBank::CartHeader::GAME_TITLE.start - FxROMBank::CartHeader::RANGE.start;

        psh::Buffer<char, 11> buf = {};
        for (u16 idx = 0; idx < 11; ++idx) {
            char cr = static_cast<char>(mmap.fx_rom.header.buf[TITLE_FIRST_IDX + idx]);
            if (!psh::is_utf8(cr)) {
                break;
            }
            buf[idx] = cr;
        }
        return buf;
    }
}  // namespace mina
