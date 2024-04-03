/// Tests for the Game Boy's memory map.
///
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#include <mina/memory_map.h>

using namespace mina;

void packed_mem() {
    static_assert(FxROMBank::CartHeader::RANGE.start == FxROMBank::InterruptVT::RANGE.end + 1);
    static_assert(FxROMBank::BANK_00.start == FxROMBank::CartHeader::RANGE.end + 1);
    static_assert(SwROMBank::RANGE.start == FxROMBank::BANK_00.end + 1);
    static_assert(VideoRAM::RANGE.start == SwROMBank::RANGE.end + 1);
    static_assert(ExtRAM::RANGE.start == VideoRAM::RANGE.end + 1);
    static_assert(FxWorkRAM::RANGE.start == ExtRAM::RANGE.end + 1);
    static_assert(SwWorkRAM::RANGE.start == FxWorkRAM::RANGE.end + 1);
    static_assert(EchoRAM::RANGE.start == SwWorkRAM::RANGE.end + 1);
    static_assert(SpriteOAM::RANGE.start == EchoRAM::RANGE.end + 1);
    static_assert(ProhibitedRegion::RANGE.start == SpriteOAM::RANGE.end + 1);
    static_assert(HwRegisterBank::RANGE.start == ProhibitedRegion::RANGE.end + 1);
    static_assert(HighRAM::RANGE.start == HwRegisterBank::RANGE.end + 1);
    static_assert(InterruptEnable::RANGE.start == HighRAM::RANGE.end + 1);

    log_fmt(LogLevel::Info, "%s test passed.", __func__);
}

void correct_sizes() {
    static_assert(sizeof(FxROMBank::InterruptVT) == 0x0100);
    static_assert(sizeof(FxROMBank::CartHeader) == 0x0050);

    static_assert(sizeof(VideoRAM::TileRAM) == 0x1800);
    static_assert(sizeof(VideoRAM::TileMap) == 0x0800);

    static_assert(sizeof(FxROMBank) == 0x4000);
    static_assert(sizeof(SwROMBank) == 0x4000);
    static_assert(sizeof(VideoRAM) == 0x2000);
    static_assert(sizeof(ExtRAM) == 0x2000);
    static_assert(sizeof(FxWorkRAM) == 0x1000);
    static_assert(sizeof(SwWorkRAM) == 0x1000);
    static_assert(sizeof(EchoRAM) == 0x1E00);
    static_assert(sizeof(SpriteOAM) == 0xA0);
    static_assert(sizeof(ProhibitedRegion) == 0x60);
    static_assert(sizeof(HwRegisterBank) == 0x80);
    static_assert(sizeof(HighRAM) == 127);
    static_assert(sizeof(InterruptEnable) == 1);
    static_assert(sizeof(MemoryMap) == 0x10000);

    log_fmt(LogLevel::Info, "%s test passed.", __func__);
}

void correct_mem_addr() {
    mina_assert(offsetof(FxROMBank, ivt) == 0x0000);
    mina_assert(offsetof(FxROMBank, header) == 0x0100);
    mina_assert(offsetof(FxROMBank, buf) == 0x0150);

    mina_assert(offsetof(MemoryMap, fx_rom) == 0x0000);
    mina_assert(offsetof(MemoryMap, sw_rom) == 0x4000);
    mina_assert(offsetof(MemoryMap, vram) == 0x8000);
    mina_assert(offsetof(MemoryMap, eram) == 0xA000);
    mina_assert(offsetof(MemoryMap, fx_wram) == 0xC000);
    mina_assert(offsetof(MemoryMap, sw_wram) == 0xD000);
    mina_assert(offsetof(MemoryMap, echo) == 0xE000);
    mina_assert(offsetof(MemoryMap, sprite) == 0xFE00);
    mina_assert(offsetof(MemoryMap, prohibited) == 0xFEA0);
    mina_assert(offsetof(MemoryMap, reg) == 0xFF00);
    mina_assert(offsetof(MemoryMap, hram) == 0xFF80);
    mina_assert(offsetof(MemoryMap, ie) == 0xFFFF);

    log_fmt(LogLevel::Info, "%s test passed.", __func__);
}

void correct_hardware_registers_addr() {
    mina_assert(offsetof(MemoryMap, reg.p1) == 0xFF00);

    mina_assert(offsetof(MemoryMap, reg.sb) == 0xFF01);
    mina_assert(offsetof(MemoryMap, reg.sc) == 0xFF02);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF03) == 0xFF03);

    mina_assert(offsetof(MemoryMap, reg.div) == 0xFF04);

    mina_assert(offsetof(MemoryMap, reg.tima) == 0xFF05);
    mina_assert(offsetof(MemoryMap, reg.tma) == 0xFF06);
    mina_assert(offsetof(MemoryMap, reg.tac) == 0xFF07);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF08_to_0xFF0E) == 0xFF08);

    mina_assert(offsetof(MemoryMap, reg.ifl) == 0xFF0F);

    mina_assert(offsetof(MemoryMap, reg.nr10) == 0xFF10);
    mina_assert(offsetof(MemoryMap, reg.nr11) == 0xFF11);
    mina_assert(offsetof(MemoryMap, reg.nr12) == 0xFF12);
    mina_assert(offsetof(MemoryMap, reg.nr13) == 0xFF13);
    mina_assert(offsetof(MemoryMap, reg.nr14) == 0xFF14);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF15) == 0xFF15);

    mina_assert(offsetof(MemoryMap, reg.nr21) == 0xFF16);
    mina_assert(offsetof(MemoryMap, reg.nr22) == 0xFF17);
    mina_assert(offsetof(MemoryMap, reg.nr23) == 0xFF18);
    mina_assert(offsetof(MemoryMap, reg.nr24) == 0xFF19);

    mina_assert(offsetof(MemoryMap, reg.nr30) == 0xFF1A);
    mina_assert(offsetof(MemoryMap, reg.nr31) == 0xFF1B);
    mina_assert(offsetof(MemoryMap, reg.nr32) == 0xFF1C);
    mina_assert(offsetof(MemoryMap, reg.nr33) == 0xFF1D);
    mina_assert(offsetof(MemoryMap, reg.nr34) == 0xFF1E);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF1F) == 0xFF1F);

    mina_assert(offsetof(MemoryMap, reg.nr41) == 0xFF20);
    mina_assert(offsetof(MemoryMap, reg.nr42) == 0xFF21);
    mina_assert(offsetof(MemoryMap, reg.nr43) == 0xFF22);
    mina_assert(offsetof(MemoryMap, reg.nr44) == 0xFF23);

    mina_assert(offsetof(MemoryMap, reg.nr50) == 0xFF24);
    mina_assert(offsetof(MemoryMap, reg.nr51) == 0xFF25);
    mina_assert(offsetof(MemoryMap, reg.nr52) == 0xFF26);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF27_to_0xFF2F) == 0xFF27);

    mina_assert(offsetof(MemoryMap, reg.wav00) == 0xFF30);
    mina_assert(offsetof(MemoryMap, reg.wav01) == 0xFF31);
    mina_assert(offsetof(MemoryMap, reg.wav02) == 0xFF32);
    mina_assert(offsetof(MemoryMap, reg.wav03) == 0xFF33);
    mina_assert(offsetof(MemoryMap, reg.wav04) == 0xFF34);
    mina_assert(offsetof(MemoryMap, reg.wav05) == 0xFF35);
    mina_assert(offsetof(MemoryMap, reg.wav06) == 0xFF36);
    mina_assert(offsetof(MemoryMap, reg.wav07) == 0xFF37);
    mina_assert(offsetof(MemoryMap, reg.wav08) == 0xFF38);
    mina_assert(offsetof(MemoryMap, reg.wav09) == 0xFF39);
    mina_assert(offsetof(MemoryMap, reg.wav10) == 0xFF3A);
    mina_assert(offsetof(MemoryMap, reg.wav11) == 0xFF3B);
    mina_assert(offsetof(MemoryMap, reg.wav12) == 0xFF3C);
    mina_assert(offsetof(MemoryMap, reg.wav13) == 0xFF3D);
    mina_assert(offsetof(MemoryMap, reg.wav14) == 0xFF3E);
    mina_assert(offsetof(MemoryMap, reg.wav15) == 0xFF3F);

    mina_assert(offsetof(MemoryMap, reg.lcdc) == 0xFF40);
    mina_assert(offsetof(MemoryMap, reg.stat) == 0xFF41);
    mina_assert(offsetof(MemoryMap, reg.scy) == 0xFF42);
    mina_assert(offsetof(MemoryMap, reg.scx) == 0xFF43);
    mina_assert(offsetof(MemoryMap, reg.ly) == 0xFF44);
    mina_assert(offsetof(MemoryMap, reg.lyc) == 0xFF45);

    mina_assert(offsetof(MemoryMap, reg.dma) == 0xFF46);
    mina_assert(offsetof(MemoryMap, reg.bgp) == 0xFF47);
    mina_assert(offsetof(MemoryMap, reg.obp0) == 0xFF48);
    mina_assert(offsetof(MemoryMap, reg.obp1) == 0xFF49);

    mina_assert(offsetof(MemoryMap, reg.wy) == 0xFF4A);
    mina_assert(offsetof(MemoryMap, reg.wx) == 0xFF4B);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF4C) == 0xFF4C);

    mina_assert(offsetof(MemoryMap, reg.key1) == 0xFF4D);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF4E) == 0xFF4E);

    mina_assert(offsetof(MemoryMap, reg.vkb) == 0xFF4F);
    mina_assert(offsetof(MemoryMap, reg.boot) == 0xFF50);
    mina_assert(offsetof(MemoryMap, reg.hdma1) == 0xFF51);
    mina_assert(offsetof(MemoryMap, reg.hdma2) == 0xFF52);
    mina_assert(offsetof(MemoryMap, reg.hdma3) == 0xFF53);
    mina_assert(offsetof(MemoryMap, reg.hdma4) == 0xFF54);
    mina_assert(offsetof(MemoryMap, reg.hdma5) == 0xFF55);

    mina_assert(offsetof(MemoryMap, reg.rp) == 0xFF56);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF57_to_0xFF67) == 0xFF57);

    mina_assert(offsetof(MemoryMap, reg.bcps) == 0xFF68);
    mina_assert(offsetof(MemoryMap, reg.bpcd) == 0xFF69);

    mina_assert(offsetof(MemoryMap, reg.ocps) == 0xFF6A);
    mina_assert(offsetof(MemoryMap, reg.ocpd) == 0xFF6B);
    mina_assert(offsetof(MemoryMap, reg.opri) == 0xFF6C);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF6D_to_0xFF6F) == 0xFF6D);

    mina_assert(offsetof(MemoryMap, reg.svbk) == 0xFF70);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF71_to_0xFF75) == 0xFF71);

    mina_assert(offsetof(MemoryMap, reg.pcm12) == 0xFF76);
    mina_assert(offsetof(MemoryMap, reg.pcm34) == 0xFF77);

    mina_assert(offsetof(MemoryMap, reg.unused_0xFF78_to_0xFF7F) == 0xFF78);

    log_fmt(LogLevel::Info, "%s test passed.", __func__);
}

int main() {
    packed_mem();
    correct_sizes();
    correct_mem_addr();
    correct_hardware_registers_addr();
    log(LogLevel::Info, "Test passed.");
}
