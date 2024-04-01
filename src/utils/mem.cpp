/// Implementation of the mina/utils/mem.h header file.
///
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

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
