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
/// Description: Starting point for the Mina Emulator.
/// Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>

#if defined(MINA_DEBUG)
#    define PSH_DEBUG
#endif

#include <mina/cartridge.h>
#include <mina/cpu/dmg.h>
#include <mina/gfx/buffer.h>
#include <mina/gfx/command.h>
#include <mina/gfx/context.h>
#include <mina/gfx/data.h>
#include <mina/gfx/swap_chain.h>
#include <mina/meta/info.h>
#include <mina/window.h>
#include <psh/assert.h>
#include <psh/input.h>
#include <psh/memory_manager.h>
#include <psh/types.h>

#include <cstdio>

using namespace mina;

struct Emulator {
    psh::MemoryManager memory_manager;
    psh::Arena         cart_arena;
    psh::Arena         gfx_arena;
    psh::Arena         frame_arena;
    psh::Arena         work_arena;
    FrameMemory        frame_memory;
    GraphicsContext    gfx_context;
    Window             win;
    CPU                cpu;
    Cartridge          cart;

    static constexpr usize MAX_MEMORY_SIZE       = psh_mebibytes(64);
    static constexpr usize MAX_CART_MEMORY_SIZE  = psh_mebibytes(8);
    static constexpr usize MAX_GFX_MEMORY_SIZE   = psh_mebibytes(20);
    static constexpr usize MAX_FRAME_MEMORY_SIZE = psh_mebibytes(18);
    static constexpr usize MAX_WORK_MEMORY_SIZE  = psh_mebibytes(17);
};

void init_emu(Emulator& emu) noexcept {
    // Initialize the memory system.
    {
        emu.memory_manager.init(Emulator::MAX_MEMORY_SIZE);

        emu.cart_arena  = emu.memory_manager.make_arena(Emulator::MAX_CART_MEMORY_SIZE).demand();
        emu.gfx_arena   = emu.memory_manager.make_arena(Emulator::MAX_GFX_MEMORY_SIZE).demand();
        emu.frame_arena = emu.memory_manager.make_arena(Emulator::MAX_FRAME_MEMORY_SIZE).demand();
        emu.work_arena  = emu.memory_manager.make_arena(Emulator::MAX_WORK_MEMORY_SIZE).demand();
    }

    // Initialize graphics application.
    {
        init_window(emu.win, {.user_pointer = &emu});

        init_graphics_system(emu.gfx_context, emu.win.handle, &emu.gfx_arena, &emu.work_arena);

        display_window(emu.win);
    }

    emu.frame_memory =
        create_frame_memory(emu.memory_manager, aspect_ratio(emu.gfx_context.swap_chain));
}

FrameStatus render_scene(
    GraphicsContext&   ctx,
    FrameMemory const& frame_memory,
    FrameResources&    resources) noexcept {
    // Prepare the frame for the pipeline commands.
    {
        FrameStatus prep_st = prepare_frame_for_rendering(ctx.dev, ctx.swap_chain, resources);

        // In case of failure, return the status to the caller.
        if (prep_st != FrameStatus::OK) {
            return prep_st;
        }
    }

    // Record and submit all commands.
    {
        // Transfer the whole staging buffer data to the device buffer.
        record_transfer_commands(
            resources.transfer_cmd,
            transfer_whole_info(
                ctx.buffers.host.handle,
                ctx.buffers.device.handle,
                VERTEX_BUF_SIZE,
                UNIFORM_BUF_SIZE));

        // Submit the transfer as soon as possible.
        submit_transfer_commands(
            ctx.queues.graphics_queue,
            resources.transfer_cmd,
            resources.transfer_ended_fence);

        record_graphics_commands(
            resources.graphics_cmd,
            ctx.queues,
            GraphicsCmdInfo{
                .pipeline                   = ctx.pipelines.graphics.handle,
                .pipeline_layout            = ctx.pipelines.graphics.pipeline_layout,
                .image                      = resources.image,
                .render_pass                = ctx.pipelines.graphics.render_pass,
                .frame_buf                  = resources.frame_buf,
                .surface_extent             = ctx.swap_chain.extent,
                .vertex_buf                 = ctx.buffers.device.handle,
                .vertex_buf_binding         = VERTEX_BINDING_DESCRIPTION.binding,
                .uniform_buf_descriptor_set = ctx.descriptor_sets.uniform_buf_descriptor_set,
                .uniform_buf_offset         = 0,
            },
            render_data_info(frame_memory));

        // Wait for the data transfer to be completed before going to the graphics pipeline.
        wait_transfer_completion(ctx.dev, resources.transfer_ended_fence);

        // Pass the new data through the graphics pipeline.
        submit_graphics_commands(
            ctx.queues.graphics_queue,
            resources.graphics_cmd,
            resources.image_available_semaphore,
            resources.render_pass_ended_semaphore,
            resources.frame_in_flight_fence);
    }

    return FrameStatus::OK;
}

void run_emu(Emulator& emu, psh::StringView cart_path) noexcept {
    switch (init_cartridge(emu.cart, &emu.cart_arena, cart_path)) {
        case psh::FileStatus::OK: {
            psh_info("Cartridge data successfully loaded.");
            break;
        }
        case psh::FileStatus::OUT_OF_MEMORY: {
            psh_fatal_fmt("Not enough memory to read the cartridge data.");
            return;
        }
        default: {
            psh_fatal_fmt("Unable to read cartridge data %s", cart_path.data.buf);
            return;
        }
    }

    transfer_fixed_rom_bank(emu.cart, emu.cpu.mmap);
    // TODO(luiz): transfer the remaining memory regions.

    // Update the window title adding the game title.
    {
        auto        cart_title = extract_cart_title(emu.cpu.mmap);
        psh::String win_title{&emu.work_arena, cart_title.size() + EMU_NAME.size()};
        psh::Status res = win_title.join(
            {
                psh::StringView{EMU_NAME.buf, EMU_NAME.size()},
                psh::StringView{cart_title.buf, cart_title.size()},
            },
            " - ");

        if (res == psh::Status::OK) {
            set_window_title(emu.win, win_title.data.buf);
        } else {
            set_window_title(emu.win, EMU_NAME.buf);
        }
    }

    while (psh_likely(!emu.win.should_close)) {
        process_input_events(emu.win);

        run_cpu_cycle(emu.cpu);

        // Graphics pipeline.
        {
            stage_host_data(
                emu.gfx_context.alloc,
                emu.gfx_context.buffers.host_buffer(),
                memory_staging_info(emu.frame_memory));

            FrameResources resources = current_frame_resources(emu.gfx_context);

            // Render the frame.
            {
                FrameStatus frame_st = render_scene(emu.gfx_context, emu.frame_memory, resources);

                switch (frame_st) {
                    case FrameStatus::OK:                     break;
                    case FrameStatus::NOT_READY:              continue;
                    case FrameStatus::SWAP_CHAIN_OUT_OF_DATE: {
                        recreate_swap_chain_context(emu.gfx_context, emu.win);
                        continue;
                    }
                    case FrameStatus::FATAL: psh_todo_msg("Handle frame preparation failure");
                }
            }

            // Present the frame.
            {
                PresentStatus present_st = present_frame(
                    emu.gfx_context.swap_chain,
                    emu.win,
                    emu.gfx_context.queues.present_queue,
                    resources.render_pass_ended_semaphore);

                switch (present_st) {
                    case PresentStatus::OK:                     break;
                    case PresentStatus::NOT_READY:              continue;
                    case PresentStatus::SWAP_CHAIN_OUT_OF_DATE: {
                        recreate_swap_chain_context(emu.gfx_context, emu.win);
                        continue;
                    }
                    default: psh_todo_msg("Handle presentation failure");
                }
            }
        }
    }
}

void terminate_emu(Emulator& emu) noexcept {
    destroy_graphics_system(emu.gfx_context);
    destroy_window(emu.win);
}

int main(i32 argc, strptr argv[]) {
    Emulator emu;
    init_emu(emu);

    psh_assert_msg(argc > 1, "Please provide the path of a ROM file as a CLI argument");
    run_emu(emu, psh::StringView{argv[1]});

    terminate_emu(emu);
    return 0;
}
