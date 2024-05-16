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
/// Description: Implementation of the Game Boy emulator instance.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>

#include <mina/gb.h>

#include <mina/gfx/context.h>
#include <mina/gfx/pipeline.h>
#include <mina/gfx/swap_chain.h>
#include <mina/memory_map.h>
#include <mina/meta/info.h>

namespace mina {
    // Implementation details of the game boy struct.
    namespace {
        [[nodiscard]] psh::FileStatus load_rom(GameBoy* gb, psh::StringView cart_path) noexcept {
            psh::FileStatus cart_valid = gb->cart.init(&gb->rom_arena, cart_path);
            return cart_valid;
        }

        psh::Buffer<char, FxROMBank::CartHeader::GAME_TITLE.size()> extract_cart_title(
            GameBoy* gb) noexcept {
            constexpr usize TITLE_FIRST_IDX =
                FxROMBank::CartHeader::GAME_TITLE.start - FxROMBank::CartHeader::RANGE.start;

            psh::Buffer<char, 11> buf{};
            for (u16 idx = 0; idx < 11; ++idx) {
                char const cr =
                    static_cast<char>(gb->cpu.mmap.fx_rom.header.buf[TITLE_FIRST_IDX + idx]);
                if (!psh::is_utf8(cr)) {
                    break;
                }
                buf[idx] = cr;
            }
            return buf;
        }
    }  // namespace

    GameBoy::GameBoy() noexcept {
        memory_manager.init(psh_mebibytes(64));

        // Initialize arenas.
        rom_arena =
            memory_manager.make_arena(psh_mebibytes(8)).demand("Unable to create ROM arena");
        gfx_arena = memory_manager.make_arena(psh_mebibytes(20))
                        .demand("Unable to create persistent arena");
        frame_arena =
            memory_manager.make_arena(psh_mebibytes(18)).demand("Unable to create frame arena");
        work_arena =
            memory_manager.make_arena(psh_mebibytes(17)).demand("Unable to create work arena");

        gfx::init_graphics_context(
            graphics_context,
            gfx::GraphicsContextConfig{
                .persistent_arena = &gfx_arena,
                .work_arena       = &work_arena,
                .win_config{
                    .user_pointer = this,
                },
            });
    }

    GameBoy::~GameBoy() noexcept {
        gfx::destroy_graphics_context(graphics_context);
    }

    void GameBoy::run(psh::StringView cart_path) noexcept {
        graphics_context.window.show();

        switch (load_rom(this, cart_path)) {
            case psh::FileStatus::OK: {
                psh::log(psh::LogLevel::Info, "Cartridge data successfully loaded.");
                break;
            }
            case psh::FileStatus::OutOfMemory: {
                psh::log_fmt(psh::LogLevel::Fatal, "Not enough memory to read the cartridge data.");
                return;
            }
            default: {
                psh::log_fmt(
                    psh::LogLevel::Fatal,
                    "Unable to read cartridge data %s",
                    cart_path.data.buf);
                return;
            }
        }

        transfer_fixed_rom_bank(cart, cpu.mmap);
        // TODO(luiz): transfer the remaining memory regions.

        // Try to update the window title.
        {
            auto const  cart_title = extract_cart_title(this);
            psh::String win_title{&work_arena, cart_title.size() + EMU_NAME.size()};
            psh::Status res = win_title.join(
                {
                    psh::StringView{EMU_NAME.buf, EMU_NAME.size()},
                    psh::StringView{cart_title.buf, cart_title.size()},
                },
                " - ");
            if (res == psh::Status::OK) {
                graphics_context.window.set_title(win_title.data.buf);
            } else {
                graphics_context.window.set_title(EMU_NAME.buf);
            }
        }

        while (!graphics_context.window.should_close()) {
            graphics_context.window.poll_events();
            if (psh_likely(gfx::prepare_frame_for_rendering(graphics_context))) {
                gfx::submit_graphics_commands(graphics_context);
                psh_assert(gfx::present_frame(graphics_context));
            }
            cpu.run_cycle();
        }
    }
}  // namespace mina
