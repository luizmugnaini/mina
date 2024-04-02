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
/// Description: Implementation of the mina/utils/mem.h header file.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/utils/mem.h>

#include <mina/base.h>

#include <cstring>

namespace mina {
    void memory_set(FatPtr<u8> fat_ptr, i32 fill) noexcept {
        if (fat_ptr.buf == nullptr) {
            return;
        }
        mina_unused(std::memset(fat_ptr.buf, fill, fat_ptr.size));
    }

    void memory_copy(u8* dest, u8 const* src, usize size) noexcept {
        if (dest == nullptr || src == nullptr) {
            return;
        }
#if defined(DEBUG) || defined(MINA_CHECK_MEMCPY_OVERLAP)
        auto const dest_addr = reinterpret_cast<uptr>(dest);
        auto const src_addr  = reinterpret_cast<uptr>(src);
        mina_assert_msg(
            (dest_addr + size > src_addr) || (dest_addr < src_addr + size),
            "memcpy called but source and destination overlap, which produces UB");
#endif
        mina_unused(std::memcpy(dest, src, size));
    }

    void memory_move(u8* dest, u8 const* src, usize size) noexcept {
        if (dest == nullptr || src == nullptr) {
            return;
        }
        mina_unused(std::memmove(dest, src, size));
    }
}  // namespace mina
