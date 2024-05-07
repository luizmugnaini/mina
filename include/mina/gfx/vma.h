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
/// Description: Inclusion header for the VulkanMemoryAllocator library.
/// Author: Luiz G. Mugnaini A. <luizmuganini@gmail.com>
///
/// Always use this header instead of `#include <vk_mem_alloc.h>` directly.

#pragma once

// Windows-related defines.
#if defined(_WIN32)
#    if !defined(NOMINMAX)
#        define NOMINMAX
#    endif
#    if !defined(WIN32_LEAN_AND_MEAN)
#        define WIN32_LEAN_AND_MEAN
#    endif
#    if !defined(VK_USE_PLATFORM_WIN32_KHR)
#        define VK_USE_PLATFORM_WIN32_KHR
#    endif  // VK_USE_PLATFORM_WIN32_KHR
#endif

// Clang will scream in disgust at VulkanMemoryAllocator if we don't silence these warnings.
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wold-style-cast"
#    pragma clang diagnostic ignored "-Wunused-function"
#    pragma clang diagnostic ignored "-Wunused-variable"
#    pragma clang diagnostic ignored "-Wsign-conversion"
#    pragma clang diagnostic ignored "-Wcovered-switch-default"
#    pragma clang diagnostic ignored "-Wnullability-extension"
#    pragma clang diagnostic ignored "-Wnullability-completeness"
#endif  // __clang__

#include <vk_mem_alloc.h>

#if defined(__clang__)
#    pragma clang diagnostic pop
#endif  // __clang__
