# Mina - An Experimental Game Boy Emulator

This is an experimental Game Boy emulator implementation.

> 🚧 This project is at its early stages of development 🚧

## Dependencies

The Presheaf game engine development requires the following dependencies to be installed:

- [CMake](https://cmake.org/) (version 3.22 or greater).
- Any of the three major modern C++20 compilers: Clang, GCC, MSVC.

Additionally, the following dependencies are vendored with the engine:

- [Presheaf](https://github.com/luizmugnaini/presheaf) alternative to C++ STL (git submodule).
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
  GPU allocation layer for Vulkan (git submodule).
- [GLFW](https://www.glfw.org/) for windowing and input handling (cloned in build).

## Development

You can build the project in the classic way any CMake project (should) work:

```bash
cmake -S . -B build
cmake --build build
```

On the other hand you can also use the simple Python script `mk.py` that manages generating the CMake
build system, building the project, building the documentation, running tests, and running the
project binaries in general.

The script requires Python 3.10 or higher. Here are some *optional* dependencies used by default:

- [Python](https://www.python.org/) (version 3.10 or greater).
- [Ninja](https://ninja-build.org/) (version 1.11.1 or greater).
- [Mold](https://github.com/rui314/mold) (version 2.4 or greater).
- [codespell](https://github.com/codespell-project/codespell).

These defaults may be easily changed by tweaking their corresponding constants at the top of `mk.py`.

If you run the `python mk.py --help` command, you'll be prompted with all of the functionality of the
script.

## OS Support

Both Windows and Linux are currently supported. I don't currently plan to support MacOS.
