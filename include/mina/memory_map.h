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
/// Description: Game Boy's memory map.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#pragma once

#include <mina/cart.h>
#include <psh/mem_utils.h>
#include <psh/types.h>

namespace mina {
    struct MemoryRange {
        usize const start;  ///< Inclusive start address.
        usize const end;    ///< Inclusive end address.

        constexpr MemoryRange(usize start, usize end) noexcept : start{start}, end{end} {}
        constexpr MemoryRange(usize start_end) noexcept : start{start_end}, end{start_end} {}

        constexpr usize size() const noexcept {
            return end - (start - 1);
        }
    };

    /// Fixed 16KB ROM Bank 0.
    ///
    /// This bank stores the ROM data from cartridge that shall be accessible throughout the
    /// whole game, usually being a fixed bank.
    struct FxROMBank {
        static constexpr MemoryRange RANGE{0x0000, 0x3FFF};

        /// 256 bit Restart and Interrupt Vector Table.
        ///
        /// - Interrupts: 0x0040, 0x0048, 0x0050, 0x0058, 0x0060.
        /// - RST instructions (1 byte): 0x0000, 0x0008, 0x0010, 0x0018, 0x0020, 0x0028, 0x0030,
        ///                              0x0038.
        struct InterruptVT {
            /// The total memory region encompassed by the ROM's interrupts.
            static constexpr MemoryRange RANGE{0x0000, 0x00FF};

            static constexpr MemoryRange VBLANK{0x0040};
            static constexpr MemoryRange STAT_LCD{0x0048};
            static constexpr MemoryRange TIMER{0x0050};
            static constexpr MemoryRange SERIAL{0x0058};
            static constexpr MemoryRange JOYPAD{0x0060};

            u8 buf[RANGE.size()]{};
        };

        /// Cartridge's header region.
        ///
        /// This is technically part of the ROM, but it'll be represented separately since it's
        /// special.
        struct CartHeader {
            /// The total memory region encompassed by the ROM's cartridge header.
            static constexpr MemoryRange RANGE{0x0100, 0x014F};

            static constexpr MemoryRange FIRST_INSTRUCTION{0x0100, 0x0103};
            static constexpr MemoryRange NINTENDO_LOGO{0x0104, 0x0133};
            static constexpr MemoryRange GAME_TITLE{0x0134, 0x013E};
            static constexpr MemoryRange GAME_DESIGNATION{0x013F, 0x0142};
            static constexpr MemoryRange COLOR_COMPATIBILITY{0x0143};
            static constexpr MemoryRange NEW_LICENSE_CODE{0x0144, 0x0145};
            static constexpr MemoryRange SGB_COMPATIBILITY{0x0146};
            static constexpr MemoryRange CARTRIGE_TYPE{0x0147};
            static constexpr MemoryRange CARTRIGE_ROM_SIZE{0x0148};
            static constexpr MemoryRange CARTRIGE_RAM_SIZE{0x0149};
            static constexpr MemoryRange DESTINATION_CODE{0x014A};
            static constexpr MemoryRange OLD_LICENSE_CODE{0x014B};
            static constexpr MemoryRange MASK_ROM_VERSION{0x014C};
            static constexpr MemoryRange COMPLEMENT_CHECKSUM{0x014D};
            static constexpr MemoryRange CHECKSUM{0x014E, 0x014F};

            u8 buf[RANGE.size()]{};
        };

        static constexpr MemoryRange BANK_00{0x0150, 0x3FFF};

        InterruptVT ivt{};
        CartHeader  header{};
        u8          buf[BANK_00.size()]{};
    };

    /// Switchable 16KB ROM Bank 01 to NN.
    ///
    /// This bank stores the ROM data from cartridge and is switchable via mapper.
    struct SwROMBank {
        static constexpr MemoryRange RANGE{0x4000, 0x7FFF};

        u8 buf[RANGE.size()]{};
    };

    /// Video RAM (VRAM).
    ///
    /// In the Game Boy, each each tile sprite has 8x8 pixels and every tile map is composed of
    /// 32x32 tiles (resulting in a total of 256x256 pixels in total). Due to the LCD screen of the
    /// console only supporting 160x144 pixels these tiles have negative offsets in both x and y
    /// coordinates.
    ///
    /// # Graphics Layers
    ///
    /// ## Background
    ///
    /// The background is composed of a matrix of tiles, called a tile map.
    ///
    /// ## Window
    ///
    /// Non-flexible second layer of non-transparent background tiles sitting on top of the
    /// background layer. It's tile positions can only be manipulated through the top-left pixel of
    /// the tile.
    ///
    /// ## Objects
    ///
    /// Objects are composed of 1 or 2 tiles and can flexibly be displayed at any position. Objects
    /// represent entities such as the main character or enemies.
    ///
    /// Objects can be combined in order to produce what we call a sprite. Each object composing a
    /// sprite is called a meta-sprite.
    struct VideoRAM {
        static constexpr MemoryRange RANGE{0x8000, 0x9FFF};

        enum struct Layer {
            Bg,   ///< Background layer.
            Win,  ///< Window layer.
            Obj,  ///< Object layer.
        };

        enum ColorID {
            c0 = 0,
            c1 = 1,
            c2 = 2,
            c3 = 3,
        };

        /// Tile RAM, or tile data.
        ///
        /// This region of memory is composed of 384 tiles, where each tile is 8x8 pixels
        /// of 2-bit color (total of 16 bytes per tile). These tiles are grouped in blocks of 128
        /// tiles each.
        ///
        /// A tile can be displayed either as part of a background map or an object (movable
        /// sprite).
        ///
        /// # Tile memory layout in the blocks
        ///
        /// Each tile has its 16 bytes consecutively laid in their corresponding address. If we have
        /// a tile T whose n-th byte (from highest to lowest) is named b_n, then T would appear as
        /// follows:
        ///
        ///                     +----+----+----+---+----+----+---+--
        ///                     |b_01|b_02|b_03|...|b_14|b_15|b16|
        ///                     +----+----+----+---+----+----+---+--
        ///                     |    |    |    |   |    |    |   |
        ///
        /// Each of the 8 lines that compose T are encoded by two consecutive bytes, for instance:
        ///
        ///             +----+----+          +----+----+               +----+----+
        ///     Line 1: |b_01|b_02|, line 2: |b_03|b_04|, ..., line 8: |b_15|b_16|
        ///             +----+----+          +----+----+               +----+----+
        ///
        /// Rather non-intuitively, the bytes of each line are swapped in order, which means that
        /// for line 1:
        ///     * b_02 is the high byte.
        ///     * b_01 is the low byte.
        ///
        /// # Color ID's (2BPP)
        ///
        /// Color ID's range from 0 to 3 (four shades of gray). Each pixel has a color depth of
        /// 2-bits (hence called 2-bits per pixel, aka 2BPP). For object sprites, the ID 0 means
        /// transparency of the corresponding pixel.
        ///
        /// Considering again tile T, let's compute the color ID's of line 1. The line 1 has its
        /// actual byte layout given by [b_02 b_01] as explained before. If we expand these bytes
        /// into their binary form, we would have something like this:
        ///     * b_01 in binary: [y_1 y_2 y_3 y_4 y_5 y_7 y_8].
        ///     * b_02 in binary: [z_1 z_2 z_3 z_4 z_5 z_7 z_8].
        /// Now we combine these bits to form the 2-bit color ID's for each pixel. From the
        /// leftmost to the rightmost pixel of the line 1 of tile T we would have:
        ///
        ///      [z_1 y_1] [z_2 y_2] [z_3 y_3] [z_4 y_4] [z_5 y_5] [z_6 y_6] [z_7 y_7] [z_8 y_8]
        ///          ^                                                                     ^
        ///          |                                                                     |
        ///    left px col ID                                                       right px col ID
        ///
        ///
        /// # Tile indexing
        ///
        /// The tiling system has two indexing modes, the unsigned mode (with ID's 0 to 255, most
        /// commonly used) and the signed mode (with ID's -128 to 127, a bit odd fashioned).
        ///
        /// The object tiles always use the unsigned indexing mode, whereas the background map tile
        /// indexing is controlled by the value of the LCDC register:
        ///     * If `HwRegisterBank::LCDC::tile_sel == 0`, signed indexing.
        ///     * If `HwRegisterBank::LCDC::tile_sel == 1`, unsigned indexing.
        struct TileRAM {
            static constexpr MemoryRange RANGE{0x8000, 0x97FF};

            static constexpr MemoryRange BLOCK_0{0x8000, 0x87FF};
            static constexpr MemoryRange BLOCK_1{0x8800, 0x8FFF};
            static constexpr MemoryRange BLOCK_2{0x9000, 0x97FF};

            /// Tiles 0 to 127 in unsigned mode.
            u8 block_0[BLOCK_0.size()]{};

            /// Tiles 128 to 255 in unsigned mode (-128 to -1 in signed mode).
            u8 block_1[BLOCK_1.size()]{};

            /// Tiles 0 to 127 in signed mode.
            u8 block_2[BLOCK_2.size()]{};

            constexpr u8* unsigned_mode_base_ptr() noexcept {
                return &block_0[0];
            }
            constexpr u8* signed_mode_base_ptr() noexcept {
                return &block_2[0];
            }

            /// The index of a tile equals the middle nibbles of the address.
            ///
            /// Example: if `0x8872` is the address of the tile, then its index is `0x87`
            static constexpr u8 unsigned_idx(u16 addr) noexcept {
                return (addr >> 4) & 0xff;
            }
        };

        /// Tile map data (aka VRAM Bank 0).
        ///
        /// The 1KiB region holds up exactly 32x32 tiles in the following way:
        ///
        /// Each byte of `TileMap` is the ID of the corresponding tile in the `TileRAM` (which may
        /// reside in any of the 3 blocks depending on the Game Boy runtime mode). The bank
        struct TileMap {
            static constexpr MemoryRange BANK_0{0x9800, 0x9BFF};
            static constexpr MemoryRange BANK_1{0x9C00, 0x9FFF};

            /// Background map attributes
            struct BGMapAttr {
                /// Color palette selection BGP (0 to 7).
                u8 col_palette : 3 = 0b000;

                /// Which bank to fetch the data:
                ///     * 0: `VideoRAM::Tilemap::bank_0`.
                ///     * 1: `VideoRAM::Tilemap::bank_1`.
                u8 bank : 1 = 0b0;

                u8 unused_ : 1 = 0b0;

                /// 0: Normal; 1: Tile vertically mirrored.
                u8 x_flipped : 1 = 0b0;

                /// 0: Normal; 1: Tile horizontally mirrored.
                u8 y_flipped : 1 = 0b0;

                /// This is a quirky bit and has to account for other register bits. For more
                /// information, please check `HwRegistersBank::LCDC::bg_en` docs.
                ///
                /// Behaviour:
                ///     * 0: Use OAM priority `SpriteOAM::Attr::Flags::priority`.
                ///     * 1: The color 1-3 of BG/window tile are drawn over the object.
                u8 priority : 1 = 0b0;
            };

            u8        bank_0[BANK_0.size()]{};  ///< Tile ID's.
            BGMapAttr bank_1[BANK_1.size()]{};  ///< [CGB only] Tile attributes.
        };

        TileRAM tile_ram{};
        TileMap tile_map{};
    };

    /// 8 KiB of external RAM (if available on the cartridge).
    struct ExtRAM {
        static constexpr MemoryRange RANGE{0xA000, 0xBFFF};

        u8 buf[RANGE.size()]{};
    };

    /// Fixed Work RAM (WRAM) Bank 0.
    struct FxWorkRAM {
        static constexpr MemoryRange RANGE{0xC000, 0xCFFF};

        u8 buf[RANGE.size()]{};
    };

    /// Switchable Work RAM (WRAM) Bank.
    ///
    /// In CGB mode this WRAM region can be swapped with banks 1 to 7.
    struct SwWorkRAM {
        static constexpr MemoryRange RANGE{0xD000, 0xDFFF};

        u8 buf[RANGE.size()]{};
    };

    /// Mirror of addresses `0xC000` to `0xDDFF`, that is, all reads and writes to this range have
    /// the same effect as reads and writes to `0xC000` to `0xDDFF`. This area has a forbidden
    /// access.
    struct EchoRAM {
        static constexpr MemoryRange RANGE{0xE000, 0xFDFF};

        u8 const buf[RANGE.size()]{};
    };

    /// Sprite Object Attribute Memory (OAM) attribute table.
    ///
    /// The table consists of 40 collections of 4 bytes (OAM) each of which is associated with a
    /// single tile.
    struct SpriteOAM {
        static constexpr MemoryRange RANGE{0xFE00, 0xFE9F};

        /// Sprite attribute.
        struct Attr {
            struct Flags {
                /// [CGB only] Chooses which of OBP (0 to 7) should be used.
                u8 cgb_palette : 3 = 0b0;

                /// [CGB only] Chooses the bank to fetch the data:
                ///     *
                ///     TODO: continue
                u8 bank : 1 = 0b0;

                /// [Non-CGB only] Chooses between OBP0 (0) and OBP1 (1).
                u8 dmg_palette : 1 = 0b0;

                u8 x_flip : 1 = 0b0;

                u8 y_flip : 1 = 0b0;

                u8 priority : 1 = 0b0;
            };

            /// The y location in screen for the tile.
            ///
            /// This position is offset by 16 pixels, that is, the tile with `y_loc = 0` is
            /// off-screen by 16 pixels in the y direction. Based on that, we have the following
            /// assertions:
            ///     * `y_loc = 0`: Object is hidden.
            ///     * `y_loc = 1`: Any 8x8 object is hidden, however an 8x16 object has its bottom 2
            ///                    lines showing.
            ///     * `y_loc = 16`: The object is placed aligned with the top line of the screen.
            ///     * `y_loc = 144`: 8x16 objects are displayed aligned with the bottom line of the
            ///                      screen.
            ///     * `y_loc = 152`: 8x8 objects are displayed aligned with the bottom line of the
            ///                      screen.
            ///     * `y_loc = 154`: The top 6 lines of the object are displayed in the bottom of
            ///                      the screen.
            ///     * `y_loc >= 160`: Object is hidden.
            u8 y_loc = 0x00;

            /// The x location in screen for the tile.
            ///
            /// This position is offset by 8 pixels, that is, the tile with `x_loc = 0` is
            /// off-screen by 8 pixels in the x direction. Since every object has a fixed width of 8
            /// pixels, it works a bit differently than `y_loc`:
            ///     * `x_loc = 0`: Object is hidden.
            ///     * `x_loc = 8`: Object is displayed aligned with the left of the screen.
            ///     * `x_loc = 152`: Object is displayed aligned with the right of the screen.
            ///     * `x_loc >= 168`: Object is hidden.
            u8 x_loc = 0x00;

            /// The number that corresponds to the tile.
            u8 tile_id = 0x00;

            /// Attributes associated to the tile.
            Flags attr{};
        };

        Attr buf[RANGE.size() / sizeof(Attr)]{};
    };

    /// Non-usable memory, its access is forbidden. This area returns `0XFF` when OAM is
    /// blocked, and otherwise the behavior depends on the hardware revision.
    struct ProhibitedRegion {
        static constexpr MemoryRange RANGE{0xFEA0, 0xFEFF};

        u8 const buf[RANGE.size()]{};
    };

    /// Hardware register bank.
    ///
    /// Responsible for holding the state of the CPU registers and I/O controllers.
    struct HwRegisterBank {
        static constexpr MemoryRange RANGE{0xFF00, 0xFF7F};

        struct P1 {
            u8 p10     : 1 = 0b0;  ///< Right or A.
            u8 p11     : 1 = 0b0;  ///< Left  or B.
            u8 p12     : 1 = 0b0;  ///< Up    or Select.
            u8 p13     : 1 = 0b0;  ///< Down  or Start.
            u8 p14     : 1 = 0b0;  ///< d-pad.
            u8 p15     : 1 = 0b0;  ///< Buttons.
            u8 unused_ : 2 = 0b00;
        };

        struct SC {
            u8 clk     : 1 = 0b0;
            u8 fast    : 1 = 0b0;
            u8 unused_ : 5 = 0b00000;
            u8 en      : 1 = 0b0;
        };

        struct TAC {
            u8 clk     : 2 = 0b00;
            u8 en      : 1 = 0b0;
            u8 unused_ : 5 = 0b00000;
        };

        struct IFL {
            u8 vblank  : 1 = 0b0;
            u8 stat    : 1 = 0b0;
            u8 timer   : 1 = 0b0;
            u8 serial  : 1 = 0b0;
            u8 joypad  : 1 = 0b0;
            u8 unused_ : 3 = 0b000;
        };

        struct LCDC {
            /// Background/Window display control.
            ///
            /// This bit has some quirks depending on the running mode of the Game Boy:
            ///     * Non-CGB: If `bg_en = 0` then both the background and window are blank (white
            ///                color), and the `win_en` bit is completely disregarded.
            ///     * CGB: If `bg_en = 0` then both the background and window layers lose their
            ///            priority to the object layer. In such case, the object is always rendered
            ///            on top of the other layers.
            ///
            ///            If `bg_en = 1`, the priority of the layers has to be calculated
            ///            considering the bit `VideoRAM::TileAttr::BGMapAttr::priority` (which I'll
            ///            abbreviate by `bgm_p`) and the sprite OAM 7th bit
            ///            `SpriteOAM::Attr::Flags::priority` (abbreviated as `oam_p`). The
            ///            following is the resulting relational table:
            ///
            ///                 +-----+-----+-----+----------------------------+
            ///                 |bg_en|oam_p|bmg_p|         Priority           |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  0  |  0  |  0  |            obj             |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  0  |  0  |  1  |            obj             |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  0  |  1  |  0  |            obj             |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  0  |  1  |  1  |            obj             |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  1  |  0  |  0  |            obj             |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  1  |  0  |  1  | bg color 1-3 otherwise obj |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  1  |  1  |  0  | bg color 1-3 otherwise obj |
            ///                 +-----+-----+-----+----------------------------+
            ///                 |  1  |  1  |  1  | bg color 1-3 otherwise obj |
            ///                 +-----+-----+-----+----------------------------+
            u8 bg_en : 1 = 0b0;

            /// Enable the rendering of the object.
            u8 obj_en : 1 = 0b0;

            /// Size of the object:
            ///     * 0: Object is 8x8.
            ///     * 1: Object is 8x16.
            u8 obj_size : 1 = 0b0;

            /// Background memory region:
            ///     * 0: `VideoRAM::TileMap` block.
            ///     * 1: `VideoRAM::TileAttr` block.
            u8 bg_map : 1 = 0b0;

            /// Background/window memory region:
            ///     * 0: `VideoRAM::TileRAM::BLOCK_0`.
            ///     * 1: `VideoRAM::TileRAM::BLOCK_1`.
            u8 win_map : 1 = 0b0;

            /// Enable the rendering of the window.
            ///
            /// For non-CGB mode, this only works if `bg_en` is set.
            u8 win_en : 1 = 0b0;

            /// Enable LCD and PPU.
            ///
            /// Changing `lcd_en` from ON (1) to OFF (0) is *strictly prohibited* outside of the
            /// VBlank period (Game Boy hardware may be damaged if so).
            u8 lcd_en : 1 = 0b0;
        };

        struct STAT {
            u8 lcd_mode : 2 = 0b00;
            u8 lyc_stat : 1 = 0b0;
            u8 intr_m0  : 1 = 0b0;
            u8 intr_m1  : 1 = 0b0;
            u8 intr_m2  : 1 = 0b0;
            u8 intr_lyc : 1 = 0b0;
        };

        struct Key1 {
            u8 en      : 1 = 0b0;
            u8 unused_ : 6 = 0b000000;
            u8 fast    : 1 = 0b0;
        };

        struct VBK {
            u8 vbk     : 2 = 0b00;
            u8 unused_ : 6 = 0b000000;
        };

        struct Boot {
            u8 off     : 1 = 0b0;
            u8 unused_ : 7 = 0b0000000;
        };

        struct SVBK {
            u8 svbk    : 1 = 0b0;
            u8 unused_ : 7 = 0b0000000;
        };

        struct PCM12 {
            u8 ch1 : 4 = 0x0;
            u8 ch2 : 4 = 0x0;
        };

        struct PCM34 {
            u8 ch3  : 4 = 0x0;
            u8 chr4 : 4 = 0x0;
        };

        P1 p1{};       ///< Joypad.

        u8 sb = 0x00;  ///< Serial transfer data.
        SC sc{};       ///< Serial transfer control.

        u8 unused_0xFF03 = 0x00;

        u8 div = 0x00;    ///< Divider register.

        u8  tima = 0x00;  ///< Time counter.
        u8  tma  = 0x00;  ///< Timer modulo.
        TAC tac{};        ///< Timer control.

        u8 unused_0xFF08_to_0xFF0E[0xFF0E - (0xFF08 - 1)]{};

        IFL ifl{};       ///< Interrupt flag.

        u8 nr10 = 0x00;  ///< Sound channel 1: sweep.
        u8 nr11 = 0x00;  ///< Sound channel 1: length timer and duty cycle.
        u8 nr12 = 0x00;  ///< Sound channel 1: volume and envelope.
        u8 nr13 = 0x00;  ///< Sound channel 1: period low.
        u8 nr14 = 0x00;  ///< Sound channel 1: period high and control.

        u8 unused_0xFF15 = 0x00;

        u8 nr21 = 0x00;  ///< Sound channel 2: length timer and duty cycle.
        u8 nr22 = 0x00;  ///< Sound channel 2: volume and envelope.
        u8 nr23 = 0x00;  ///< Sound channel 2: period low.
        u8 nr24 = 0x00;  ///< Sound channel 2: period high and control.

        u8 nr30 = 0x00;  ///< Sound channel 3: DAC enable.
        u8 nr31 = 0x00;  ///< Sound channel 3: length timer.
        u8 nr32 = 0x00;  ///< Sound channel 3: output level.
        u8 nr33 = 0x00;  ///< Sound channel 3: period low.
        u8 nr34 = 0x00;  ///< Sound channel 3: period high and control.

        u8 unused_0xFF1F = 0x00;

        u8 nr41 = 0x00;  ///< Sound channel 4: length timer.
        u8 nr42 = 0x00;  ///< Sound channel 4: volume and envelope.
        u8 nr43 = 0x00;  ///< Sound channel 4: frequency and randomness.
        u8 nr44 = 0x00;  ///< Sound channel 4: control.

        u8 nr50 = 0x00;  ///< Master volume and VIN panning.
        u8 nr51 = 0x00;  ///< Sound panning.
        u8 nr52 = 0x00;  ///< Sound ON/OFF.

        u8 unused_0xFF27_to_0xFF2F[0xFF2F - (0xFF27 - 1)]{};

        /// Storage for one of the sound channels' waveform.
        u8 wav00 = 0x00;
        u8 wav01 = 0x00;
        u8 wav02 = 0x00;
        u8 wav03 = 0x00;
        u8 wav04 = 0x00;
        u8 wav05 = 0x00;
        u8 wav06 = 0x00;
        u8 wav07 = 0x00;
        u8 wav08 = 0x00;
        u8 wav09 = 0x00;
        u8 wav10 = 0x00;
        u8 wav11 = 0x00;
        u8 wav12 = 0x00;
        u8 wav13 = 0x00;
        u8 wav14 = 0x00;
        u8 wav15 = 0x00;

        LCDC lcdc{};      ///< LCD control.
        STAT stat{};      ///< LCD status.
        u8   scy = 0x00;  ///< Viewport y position.
        u8   scx = 0x00;  ///< Viewport x position.
        u8   ly  = 0x00;  ///< LCD y coordinate.
        u8   lyc = 0x00;  ///< `ly` compare.

        u8 dma  = 0x00;   ///< OAM DMA source address and start.
        u8 bgp  = 0x00;   ///< BG palette data.
        u8 obp0 = 0x00;   ///< Object palette 0 data.
        u8 obp1 = 0x00;   ///< Object palette 1 data.

        /// The top-left coordinate of the window is (x, y) = (wx - 7, wy).
        u8 wy = 0x00;  ///< Window y position.
        u8 wx = 0x00;  ///< Window x position.

        u8 unused_0xFF4C = 0x00;

        Key1 key1{};  ///< Prepare speed switch.

        u8 unused_0xFF4E = 0x00;

        VBK  vkb{};         ///< VRAM bank.
        Boot boot{};        ///< Boot.
        u8   hdma1 = 0x00;  ///< VRAM DMA source high.
        u8   hdma2 = 0x00;  ///< VRAM DMA source low.
        u8   hdma3 = 0x00;  ///< VRAM DMA destination high.
        u8   hdma4 = 0x00;  ///< VRAM DMA destination low.
        u8   hdma5 = 0x00;  ///< VRAM DMA length/mode/start.

        u8 rp = 0x00;       ///< Infrared communication port.

        u8 unused_0xFF57_to_0xFF67[0xFF67 - (0xFF57 - 1)]{};

        u8 bcps = 0x00;  ///< Background palette index (aka background color palette specification).
        u8 bpcd = 0x00;  ///< Background palette data (aka background color palette data).

        u8 ocps = 0x00;  ///< Object palette index (aka object color palette specification).
        u8 ocpd = 0x00;  ///< Object palette data (aka object color palette data).
        u8 opri = 0x00;  ///< Object priority mode.

        u8 unused_0xFF6D_to_0xFF6F[0xFF6F - (0xFF6D - 1)]{};

        SVBK svbk{};  ///< WRAM bank.

        u8 unused_0xFF71_to_0xFF75[0xFF75 - (0xFF71 - 1)]{};

        PCM12 pcm12{};  ///< Audio digital outputs 1 and 2.
        PCM34 pcm34{};  ///< Audio digital outputs 3 and 4.

        u8 unused_0xFF78_to_0xFF7F[0xFF7F - (0xFF78 - 1)]{};
    };

    /// High RAM (HRAM).
    ///
    /// Intended as a quick RAM access region.
    struct HighRAM {
        static constexpr MemoryRange RANGE{0xFF80, 0xFFFE};

        u8 buf[RANGE.size()]{};
    };

    /// Interrupt enable register.
    ///
    /// This register is responsible for controlling the interrupt vector table.
    struct InterruptEnable {
        static constexpr MemoryRange RANGE{0xFFFF};

        u8 vblank  : 1 = 0b0;
        u8 stat    : 1 = 0b0;
        u8 timer   : 1 = 0b0;
        u8 serial  : 1 = 0b0;
        u8 joypad  : 1 = 0b0;
        u8 unused_ : 3 = 0b000;
    };

    /// Game Boy's memory map.
    ///
    /// The whole memory of the Game Boy encompasses 64 KiB in size.
    struct MemoryMap {
        FxROMBank        fx_rom{};
        SwROMBank        sw_rom{};
        VideoRAM         vram{};
        ExtRAM           eram{};
        FxWorkRAM        fx_wram{};
        SwWorkRAM        sw_wram{};
        EchoRAM          echo{};
        SpriteOAM        sprite{};
        ProhibitedRegion prohibited{};
        HwRegisterBank   reg{};
        HighRAM          hram{};
        InterruptEnable  ie{};
    };

    void transfer_fixed_rom_bank(GbCart const& cart, MemoryMap& mmap) noexcept;
}  // namespace mina
