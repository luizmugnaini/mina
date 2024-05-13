#include <mina/memory_map.h>
#include <cstring>

namespace mina {
    u8* MemoryMap::memstart() noexcept {
        return reinterpret_cast<u8*>(this);
    }

    void MemoryMap::fixed_rom_bank_transfer(GbCart const& cart) noexcept {
        constexpr usize BANK_SIZE = fx_rom.RANGE.size();
        std::memset(&fx_rom, 0xFF, BANK_SIZE);

        usize copy_size = psh_min(BANK_SIZE, cart.content.data.size);
        std::memcpy(&fx_rom, cart.content.data.buf, copy_size);
    }
}  // namespace mina
