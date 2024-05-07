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

namespace mina {
    GameBoy::GameBoy() noexcept {
        memory_manager.init(204800);
        persistent_arena =
            memory_manager.make_arena(51200).demand("Unable to create persistent arena");
        frame_arena = memory_manager.make_arena(10240).demand("Unable to create frame arena");
        work_arena  = memory_manager.make_arena(131072).demand("Unable to create work arena");
        gfx::init_graphics_context(
            gfx::GraphicsContextConfig{
                .persistent_arena = &persistent_arena,
                .work_arena       = &work_arena,
                .win_config{
                    .user_pointer = this,
                },
            },
            graphics_context);
    }

    GameBoy::~GameBoy() noexcept {
        gfx::destroy_graphics_context(graphics_context);
    }

    void GameBoy::run() noexcept {
        graphics_context.window.show();
        while (!graphics_context.window.should_close()) {
            graphics_context.window.poll_events();
            if (!gfx::prepare_frame_for_rendering(graphics_context)) {
                continue;
            }
            gfx::submit_graphics_commands(graphics_context);
            psh_assert(gfx::present_frame(graphics_context));
        }
    }
}  // namespace mina
