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
/// Description: Implementation of the Game Boy cartridge layer.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/cartridge.h>

#include <psh/streams.h>

namespace mina {
    psh::FileStatus init_cartridge(Cartridge& c, psh::Arena* arena, psh::StringView path) noexcept {
        c.path.init(arena, path);
        psh::FileReadResult res = psh::read_file(arena, c.path.data.buf, psh::ReadFileFlag::READ_BIN);
        c.content               = res.content;
        return res.status;
    }
}  // namespace mina
