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
/// Description: Tests for the Game Boy's memory map.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/memory_map.h>
#include <psh/assert.h>
#include <psh/io.h>

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

    psh::log_fmt(psh::LogLevel::Info, "%s test passed.", __func__);
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

    psh::log_fmt(psh::LogLevel::Info, "%s test passed.", __func__);
}

void correct_mem_addr() {
    psh_assert(offsetof(FxROMBank, ivt) == 0x0000);
    psh_assert(offsetof(FxROMBank, header) == 0x0100);
    psh_assert(offsetof(FxROMBank, buf) == 0x0150);

    psh_assert(offsetof(MemoryMap, fx_rom) == 0x0000);
    psh_assert(offsetof(MemoryMap, sw_rom) == 0x4000);
    psh_assert(offsetof(MemoryMap, vram) == 0x8000);
    psh_assert(offsetof(MemoryMap, eram) == 0xA000);
    psh_assert(offsetof(MemoryMap, fx_wram) == 0xC000);
    psh_assert(offsetof(MemoryMap, sw_wram) == 0xD000);
    psh_assert(offsetof(MemoryMap, echo) == 0xE000);
    psh_assert(offsetof(MemoryMap, sprite) == 0xFE00);
    psh_assert(offsetof(MemoryMap, prohibited) == 0xFEA0);
    psh_assert(offsetof(MemoryMap, reg) == 0xFF00);
    psh_assert(offsetof(MemoryMap, hram) == 0xFF80);
    psh_assert(offsetof(MemoryMap, ie) == 0xFFFF);

    psh::log_fmt(psh::LogLevel::Info, "%s test passed.", __func__);
}

void correct_hardware_registers_addr() {
    psh_assert(offsetof(MemoryMap, reg.p1) == 0xFF00);

    psh_assert(offsetof(MemoryMap, reg.sb) == 0xFF01);
    psh_assert(offsetof(MemoryMap, reg.sc) == 0xFF02);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF03) == 0xFF03);

    psh_assert(offsetof(MemoryMap, reg.div) == 0xFF04);

    psh_assert(offsetof(MemoryMap, reg.tima) == 0xFF05);
    psh_assert(offsetof(MemoryMap, reg.tma) == 0xFF06);
    psh_assert(offsetof(MemoryMap, reg.tac) == 0xFF07);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF08_to_0xFF0E) == 0xFF08);

    psh_assert(offsetof(MemoryMap, reg.ifl) == 0xFF0F);

    psh_assert(offsetof(MemoryMap, reg.nr10) == 0xFF10);
    psh_assert(offsetof(MemoryMap, reg.nr11) == 0xFF11);
    psh_assert(offsetof(MemoryMap, reg.nr12) == 0xFF12);
    psh_assert(offsetof(MemoryMap, reg.nr13) == 0xFF13);
    psh_assert(offsetof(MemoryMap, reg.nr14) == 0xFF14);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF15) == 0xFF15);

    psh_assert(offsetof(MemoryMap, reg.nr21) == 0xFF16);
    psh_assert(offsetof(MemoryMap, reg.nr22) == 0xFF17);
    psh_assert(offsetof(MemoryMap, reg.nr23) == 0xFF18);
    psh_assert(offsetof(MemoryMap, reg.nr24) == 0xFF19);

    psh_assert(offsetof(MemoryMap, reg.nr30) == 0xFF1A);
    psh_assert(offsetof(MemoryMap, reg.nr31) == 0xFF1B);
    psh_assert(offsetof(MemoryMap, reg.nr32) == 0xFF1C);
    psh_assert(offsetof(MemoryMap, reg.nr33) == 0xFF1D);
    psh_assert(offsetof(MemoryMap, reg.nr34) == 0xFF1E);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF1F) == 0xFF1F);

    psh_assert(offsetof(MemoryMap, reg.nr41) == 0xFF20);
    psh_assert(offsetof(MemoryMap, reg.nr42) == 0xFF21);
    psh_assert(offsetof(MemoryMap, reg.nr43) == 0xFF22);
    psh_assert(offsetof(MemoryMap, reg.nr44) == 0xFF23);

    psh_assert(offsetof(MemoryMap, reg.nr50) == 0xFF24);
    psh_assert(offsetof(MemoryMap, reg.nr51) == 0xFF25);
    psh_assert(offsetof(MemoryMap, reg.nr52) == 0xFF26);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF27_to_0xFF2F) == 0xFF27);

    psh_assert(offsetof(MemoryMap, reg.wav00) == 0xFF30);
    psh_assert(offsetof(MemoryMap, reg.wav01) == 0xFF31);
    psh_assert(offsetof(MemoryMap, reg.wav02) == 0xFF32);
    psh_assert(offsetof(MemoryMap, reg.wav03) == 0xFF33);
    psh_assert(offsetof(MemoryMap, reg.wav04) == 0xFF34);
    psh_assert(offsetof(MemoryMap, reg.wav05) == 0xFF35);
    psh_assert(offsetof(MemoryMap, reg.wav06) == 0xFF36);
    psh_assert(offsetof(MemoryMap, reg.wav07) == 0xFF37);
    psh_assert(offsetof(MemoryMap, reg.wav08) == 0xFF38);
    psh_assert(offsetof(MemoryMap, reg.wav09) == 0xFF39);
    psh_assert(offsetof(MemoryMap, reg.wav10) == 0xFF3A);
    psh_assert(offsetof(MemoryMap, reg.wav11) == 0xFF3B);
    psh_assert(offsetof(MemoryMap, reg.wav12) == 0xFF3C);
    psh_assert(offsetof(MemoryMap, reg.wav13) == 0xFF3D);
    psh_assert(offsetof(MemoryMap, reg.wav14) == 0xFF3E);
    psh_assert(offsetof(MemoryMap, reg.wav15) == 0xFF3F);

    psh_assert(offsetof(MemoryMap, reg.lcdc) == 0xFF40);
    psh_assert(offsetof(MemoryMap, reg.stat) == 0xFF41);
    psh_assert(offsetof(MemoryMap, reg.scy) == 0xFF42);
    psh_assert(offsetof(MemoryMap, reg.scx) == 0xFF43);
    psh_assert(offsetof(MemoryMap, reg.ly) == 0xFF44);
    psh_assert(offsetof(MemoryMap, reg.lyc) == 0xFF45);

    psh_assert(offsetof(MemoryMap, reg.dma) == 0xFF46);
    psh_assert(offsetof(MemoryMap, reg.bgp) == 0xFF47);
    psh_assert(offsetof(MemoryMap, reg.obp0) == 0xFF48);
    psh_assert(offsetof(MemoryMap, reg.obp1) == 0xFF49);

    psh_assert(offsetof(MemoryMap, reg.wy) == 0xFF4A);
    psh_assert(offsetof(MemoryMap, reg.wx) == 0xFF4B);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF4C) == 0xFF4C);

    psh_assert(offsetof(MemoryMap, reg.key1) == 0xFF4D);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF4E) == 0xFF4E);

    psh_assert(offsetof(MemoryMap, reg.vkb) == 0xFF4F);
    psh_assert(offsetof(MemoryMap, reg.boot) == 0xFF50);
    psh_assert(offsetof(MemoryMap, reg.hdma1) == 0xFF51);
    psh_assert(offsetof(MemoryMap, reg.hdma2) == 0xFF52);
    psh_assert(offsetof(MemoryMap, reg.hdma3) == 0xFF53);
    psh_assert(offsetof(MemoryMap, reg.hdma4) == 0xFF54);
    psh_assert(offsetof(MemoryMap, reg.hdma5) == 0xFF55);

    psh_assert(offsetof(MemoryMap, reg.rp) == 0xFF56);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF57_to_0xFF67) == 0xFF57);

    psh_assert(offsetof(MemoryMap, reg.bcps) == 0xFF68);
    psh_assert(offsetof(MemoryMap, reg.bpcd) == 0xFF69);

    psh_assert(offsetof(MemoryMap, reg.ocps) == 0xFF6A);
    psh_assert(offsetof(MemoryMap, reg.ocpd) == 0xFF6B);
    psh_assert(offsetof(MemoryMap, reg.opri) == 0xFF6C);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF6D_to_0xFF6F) == 0xFF6D);

    psh_assert(offsetof(MemoryMap, reg.svbk) == 0xFF70);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF71_to_0xFF75) == 0xFF71);

    psh_assert(offsetof(MemoryMap, reg.pcm12) == 0xFF76);
    psh_assert(offsetof(MemoryMap, reg.pcm34) == 0xFF77);

    psh_assert(offsetof(MemoryMap, reg.unused_0xFF78_to_0xFF7F) == 0xFF78);

    psh::log_fmt(psh::LogLevel::Info, "%s test passed.", __func__);
}

int main() {
    packed_mem();
    correct_sizes();
    correct_mem_addr();
    correct_hardware_registers_addr();
    psh::log(psh::LogLevel::Info, "Test passed.");
}
