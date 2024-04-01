/// Game Boy's memory map.
///
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#pragma once

#include <mina/base.h>
#include <mina/utils/mem.h>

namespace mina {
    struct MemoryRange {
        usize const start;  ///< Inclusive start address.
        usize const end;    ///< Inclusive end address.

        constexpr MemoryRange(usize _min, usize _max) noexcept : start{_min}, end{_max} {}
        constexpr MemoryRange(usize min_max) noexcept : start{min_max}, end{min_max} {}

        [[nodiscard]] constexpr usize size() const noexcept {
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
    struct VideoRAM {
        static constexpr MemoryRange RANGE{0x8000, 0x9FFF};

        /// RAM region used exclusively for video-related work.
        ///
        /// This region of memory is composed of 384 tiles, where each tile is 8x8 pixels
        /// of 2-bit color (total of 16 bytes).
        ///
        /// These tiles are grouped in blocks of 128 tiles each.
        ///
        /// A tile can be displayed either as part of a background map or an object (movable
        /// sprite).
        ///
        /// The tiling system has two indexing modes, the unsigned mode (with ID's 0 to 255, most
        /// commonly used) and the signed mode (with ID's -128 to 127, a bit odd fashioned).
        ///
        /// The object tiles always use the unsigned indexing mode, whereas the background map tiles
        /// can either use the signed (if `HwRegisterBank::LCDC::tile_sel == 0`)  or unsigned (if
        /// `HwRegisterBank::LCDC::tile_sel == 1`).
        struct TileRAM {
            static constexpr MemoryRange RANGE{0x8000, 0x97FF};

            static constexpr MemoryRange BLOCK_0{0x8000, 0x87FF};
            static constexpr MemoryRange BLOCK_1{0x8800, 0x8FFF};
            static constexpr MemoryRange BLOCK_2{0x9000, 0x97FF};

            u8 block_0[BLOCK_0.size()]{};  ///< Tiles 0 to 127 in unsigned mode.
            u8 block_1[BLOCK_1.size()]{};  ///< Tiles 128 to 255 in unsigned mode (-128 to -1 in
                                           ///< signed mode).
            u8 block_2[BLOCK_2.size()]{};  ///< Tiles 0 to 127 in signed mode.

            [[nodiscard]] constexpr u8* unsigned_mode_base_ptr() {
                return &block_0[0];
            }
            [[nodiscard]] constexpr u8* signed_mode_base_ptr() {
                return &block_2[0];
            }
        };

        /// 1KiB region used to build the display.
        ///
        /// Each byte represents a tile on the display, therefore the region can hold up to 32x32
        /// (1024 bytes) tiles.
        struct TileMap {
            static constexpr MemoryRange RANGE{0x9800, 0x9BFF};

            u8 buf[RANGE.size()]{};
        };

        /// Tile attributes (only available in CGB mode).
        struct TileAttr {
            static constexpr MemoryRange RANGE{0x9C00, 0x9FFF};

            u8 buf[RANGE.size()]{};
        };

        TileRAM  tile_ram{};
        TileMap  tile_map{};
        TileAttr tile_attr{};
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
            /// The x location in screen for the tile. This position is offset by 8 pixels, that is,
            /// the tile with `x_loc = 0` is off-screen by 8 pixels in the x direction.
            u8 x_loc = 0x00;

            /// The y location in screen for the tile. This position is offset by 16 pixels, that
            /// is, the tile with `y_loc = 0` is off-screen by 16 pixels in the y direction.
            u8 y_loc = 0x00;

            /// The number that corresponds to the tile.
            u8 tile_id = 0x00;

            /// Attributes associated to the tile.
            u8 attr = 0x00;
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
            u8 p11     : 1 = 0b0;  ///< Left or B.
            u8 p12     : 1 = 0b0;  ///< Up or Select.
            u8 p13     : 1 = 0b0;  ///< Down or Start.
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
            u8 bg_en    : 1 = 0b0;
            u8 obj_en   : 1 = 0b0;
            u8 obj_size : 1 = 0b0;
            u8 tile_sel : 1 = 0b0;
            u8 win_en   : 1 = 0b0;
            u8 win_map  : 1 = 0b0;
            u8 lcd_en   : 1 = 0b0;
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

        ///< Storage for one of the sound channels' waveform.
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

        u8 wy = 0x00;     ///< Window y position.
        u8 wx = 0x00;     ///< Window x position.

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
}  // namespace mina
