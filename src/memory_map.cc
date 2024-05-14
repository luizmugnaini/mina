#include <mina/memory_map.h>
#include <cstring>

namespace mina {
    void transfer_fixed_rom_bank(GbCart const& cart, MemoryMap& mmap) noexcept {
        constexpr usize BANK_SIZE = FxROMBank::RANGE.size();
        std::memset(&mmap.fx_rom, 0xFF, BANK_SIZE);

        usize copy_size = psh_min(BANK_SIZE, cart.content.data.size);
        std::memcpy(&mmap.fx_rom, cart.content.data.buf, copy_size);
    }
}  // namespace mina
