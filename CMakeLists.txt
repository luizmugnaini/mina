# ------------------------------------------------------------------------------
# Project description
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.22)

project(
    mina
    VERSION 0.0.1
    LANGUAGES CXX C
)

# ---- Compilation config ----

message(STATUS "C Compiler executable: ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler executable: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker executable: ${CMAKE_LINKER}")

# - Compilation options for the codebase -

option(MINA_DISABLE_LOGGING "Compile without logging" OFF)
option(MINA_DISABLE_ASSERTS "Compile without asserts" OFF)

option(MINA_DEBUG "Compile in debug mode" ON)
option(MINA_CHECK_BOUNDS "Compile with bound check" ON)
option(MINA_CHECK_MEMCPY_OVERLAP "Checks if memcpy is called with overlapping regions of memory" ON)
option(MINA_ENABLE_VULKAN_VALIDATION_LAYERS "Enable Vulkan validation layers" ON)

# ------------------------------------------------------------------------------
# Misc.
# ------------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling tests")
    enable_testing()
endif()

# Enable Clangd LSP integration with the project
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Compiler info
# ------------------------------------------------------------------------------

message(STATUS "C Compiler executable: ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler executable: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker executable: ${CMAKE_LINKER}")

# ------------------------------------------------------------------------------
# GCC configuration
# ------------------------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID STREQUAL "GCC")
    set(
        MINA_CXX_FLAGS
        "-std=c++20"
        "-Wall"
        "-Wextra"
        "-pedantic"
        "-Wuninitialized"
        "-Wswitch"
        "-Wcovered-switch-default"
        "-Wshadow"             # Warn the user if a variable declaration shadows a parent variable.
        "-Wnon-virtual-dtor"   # Warn the user if a class with virtual functions has a
                               # Non-virtual destructor. This helps catch hard to
                               # Track down memory errors.
        "-Wold-style-cast"     # Warn for c-style casts.
        "-Wcast-align"         # Warn for potential performance problem casts.
        "-Wunused"             # Warn on anything being unused.
        "-Woverloaded-virtual" # Warn if you overload (not override) a virtual
                               # Function.
        "-Wpedantic"           # Warn if non-standard C++ is used.
        "-Wconversion"         # Warn on type conversions that may lose data.
        "-Wsign-conversion"    # Warn on sign conversions.
        "-Wnull-dereference"   # Warn if a null dereference is detected.
        "-Wdouble-promotion"   # Warn if float is implicit promoted to double.
        "-Wmisleading-indentation" # Warn if indentation implies blocks where blocks
                                   # do not exist.

        # Don't warn about non-literal format strings.
        # TODO: we should use -Wformat=2
        # "-Wno-format-security"
        # "-Wno-format-nonliteral"

        "-fno-rtti"               # Disable runtime type inference.
        "-fno-exceptions"         # Disable exceptions.
        "-fno-cxx-exceptions"     # Disable exceptions.
        "-fcolor-diagnostics"     # Enable colors in the diagnostics.
        "-fno-force-emit-vtables" # Disable forcing vtables for modules where it isn't necessary.
    )
    set(
        MINA_CXX_SAN_FLAGS
        "-fsanitize=address"           # - Enable AddressSanitizer, a fast memory error detector.
                                       #   Memory access instructions are instrumented to detect
                                       #   out-of-bounds and use-after-free bugs. The option
                                       #   enables -fsanitize-address-use-after-scope.
        "-fsanitize=pointer-compare"   # - Instrument comparison operation (<, <=, >, >=) with
                                       #   pointer operands. The option must be combined with
                                       #   either -fsanitize=kernel-address or -fsanitize=address
        "-fsanitize=pointer-subtract"  # - Instrument subtraction with pointer operands.
                                       #   The option must be combined with either
                                       #   -fsanitize=kernel-address or -fsanitize=address.
        "-fsanitize=shadow-call-stack" # - Enable ShadowCallStack, a security enhancement
                                       #   mechanism used to protect programs against return
                                       #   address overwrites (e.g. stack buffer overflows.)
        "-fsanitize=undefined"         # - Enable UndefinedBehaviorSanitizer.
        "-fstack-protector-strong"     # - Like -fstack-protector but includes additional functions
                                       #   to be protected — those that have local array definitions,
                                       #   or have references to local frame addresses.
        "-fsanitize=leak"              # - Enable LeakSanitizer, a memory leak detector. This option only
                                       #   matters for linking of executables.
    )
endif()

# ------------------------------------------------------------------------------
# Clang configuration
# ------------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # ---- Clang-tidy integration ----

    option(USE_TIDY "Use the clang-tidy tool for linting" OFF)
    if(USE_TIDY)
        set(CMAKE_CXX_CLANG_TIDY "clang-tidy" "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
    endif()

    if(UNIX)
        set(
            MINA_CXX_FLAGS
            "-std=c++20"
            "-Wall"
            "-Wextra"
            "-pedantic"
            "-Wuninitialized"
            "-Wswitch"
            "-Wcovered-switch-default"
            "-Wshadow"             # Warn the user if a variable declaration shadows a parent variable.
            "-Wnon-virtual-dtor"   # Warn the user if a class with virtual functions has a
                                   # Non-virtual destructor. This helps catch hard to
                                   # Track down memory errors.
            "-Wold-style-cast"     # Warn for c-style casts.
            "-Wcast-align"         # Warn for potential performance problem casts.
            "-Wunused"             # Warn on anything being unused.
            "-Woverloaded-virtual" # Warn if you overload (not override) a virtual
                                   # Function.
            "-Wpedantic"           # Warn if non-standard C++ is used.
            "-Wconversion"         # Warn on type conversions that may lose data.
            "-Wsign-conversion"    # Warn on sign conversions.
            "-Wnull-dereference"   # Warn if a null dereference is detected.
            "-Wdouble-promotion"   # Warn if float is implicit promoted to double.
            "-Wmisleading-indentation" # Warn if indentation implies blocks where blocks
                                       # do not exist.

            # Don't warn about non-literal format strings.
            # TODO: we should use -Wformat=2
            "-Wno-format-security"
            "-Wno-format-nonliteral"

            "-fno-rtti"               # Disable runtime type inference.
            "-fno-exceptions"         # Disable exceptions.
            "-fno-cxx-exceptions"     # Disable exceptions.
            "-fcolor-diagnostics"     # Enable colors in the diagnostics.
            "-fno-force-emit-vtables" # Disable forcing vtables for modules where it isn't necessary.
        )
        set(
            MINA_CXX_SAN_FLAGS
            "-fsanitize=address"           # - Enable AddressSanitizer, a fast memory error detector.
                                           #   Memory access instructions are instrumented to detect
                                           #   out-of-bounds and use-after-free bugs. The option
                                           #   enables -fsanitize-address-use-after-scope.
            "-fsanitize=pointer-compare"   # - Instrument comparison operation (<, <=, >, >=) with
                                           #   pointer operands. The option must be combined with
                                           #   either -fsanitize=kernel-address or -fsanitize=address
            "-fsanitize=pointer-subtract"  # - Instrument subtraction with pointer operands.
                                           #   The option must be combined with either
                                           #   -fsanitize=kernel-address or -fsanitize=address.
            "-fsanitize=shadow-call-stack" # - Enable ShadowCallStack, a security enhancement
                                           #   mechanism used to protect programs against return
                                           #   address overwrites (e.g. stack buffer overflows.)
            "-fsanitize=undefined"         # - Enable UndefinedBehaviorSanitizer.
            "-fstack-protector-strong"     # - Like -fstack-protector but includes additional functions
                                           #   to be protected — those that have local array definitions,
                                           #   or have references to local frame addresses.
            "-fsanitize=leak"              # - Enable LeakSanitizer, a memory leak detector. This option only
                                           #   matters for linking of executables.
        )
    elseif(MSVC)
        # Flags for the clang-cl compiler (see https://clang.llvm.org/docs/UsersManual.html#clang-cl).
        message(STATUS "Using MSVC-like compiler flags")
        set(
            MINA_CXX_FLAGS
            "/std:c++20"

            "/Ob2"          # Inline functions as deemed beneficial by the compiler.
            "/GR-"          # Disable emission of RTTI data.
            "-nobuiltininc" # Disable builtin #include directories.
            "/fp:except-"   # Disable exceptions.

            "-fcolor-diagnostics"

            "/W3"
            "-Wuninitialized"
            "-Wswitch"
            "-Wcovered-switch-default"
            "-Wshadow"
            "-Wnon-virtual-dtor"
            "-Wold-style-cast"
            "-Wunused"
            "-Woverloaded-virtual"
            "-Wpedantic"
            "-Wconversion"
            "-Wsign-conversion"
            "-Wnull-dereference"
            "-Wdouble-promotion"
            "-Wmisleading-indentation"
            "-Wcast-align"
            "-Wno-cast-align"
            "-Wno-language-extension-token"

            # Don't warn about non-literal format strings.
            # TODO: we should use -Wformat=2
            "-Wno-format-security"
            "-Wno-format-nonliteral"
        )
        set(MINA_CXX_SAN_FLAGS "")  # TODO: Enable ASan and UBSan
    endif() # MSVC Clang
endif() # Clang

# ------------------------------------------------------------------------------
# MSVC configuration
# ------------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(
        MINA_CXX_FLAGS
        "/std:c++20"
        "/W3"
        "/EHc"  # Extern "C" defaults to nothrow.
        "/GA"   # Optimize for Windows applications.
        "/GR-"  # Disable RTTI information
    )
    set(MINA_CXX_SAN_FLAGS "")
endif()

if(MSVC)
    # Set debug/release flags for MSVC and Clang-cl
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "Compiling with debug flags")
            set(
                MINA_CXX_FLAGS ${MINA_CXX_FLAGS}
                "/Zi"   # Debug info.
                "/Ob0"  # Disable inlining.
                "/Oy-"  # Disable frame pointer optimization.
            )
        else()
            set(
                MINA_CXX_FLAGS ${MINA_CXX_FLAGS}
                "/Ob2"  # Inline functions as deemed beneficial by the compiler.
            )
        endif()
endif()

# ------------------------------------------------------------------------------
# Set the codebase options as compiler flags with -D or /D
# ------------------------------------------------------------------------------

if(UNIX)
    set(DEFINE_MACRO "-D")
elseif(MSVC)
    set(DEFINE_MACRO "/D")

     # Says that fopen is unsafe... cringe
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}_CRT_SECURE_NO_WARNINGS")
endif()

if(MINA_DEBUG)
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}MINA_DEBUG")
endif()
if(MINA_CHECK_BOUNDS)
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}MINA_CHECK_BOUNDS")
endif()
if(MINA_MEMCPY_OVERLAP)
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}MINA_MEMCPY_OVERLAP")
endif()
if(MINA_CHECK_SHADER_COMPILATION)
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}MINA_CHECK_SHADER_COMPILATION")
endif()
if(MINA_CHECK_PROGRAM_LINKING)
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}MINA_CHECK_PROGRAM_LINKING")
endif()
if(MINA_DISABLE_LOGGING)
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}MINA_DISABLE_LOGGING")
endif()
if(MINA_DISABLE_ASSERTS)
    set(MINA_CXX_FLAGS "${MINA_CXX_FLAGS}" "${DEFINE_MACRO}MINA_DISABLE_ASSERTS")
endif()

message(STATUS "Mina C++ compiler flags: ${MINA_CXX_FLAGS}")
message(STATUS "Mina C++ linker flags: ${MINA_CXX_SAN_FLAGS}")

# ------------------------------------------------------------------------------
# Third-party dependencies
# ------------------------------------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

# - Vulkan -

find_package(Vulkan REQUIRED)

# - GLFW -

set(GLFW_BUILD_TESTS OFF)
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_WAYLAND OFF)  # TODO: support wayland
add_subdirectory("${THIRDPARTY_DIR}/glfw")

# ------------------------------------------------------------------------------
# Source files
# ------------------------------------------------------------------------------

set(
    MINA_ENGINE_SRC
    "${CMAKE_SOURCE_DIR}/src/base.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/mem.cpp"
)

set(
    MINA_EXE_SRC
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
)

# ------------------------------------------------------------------------------
# Mina main binary and library
# ------------------------------------------------------------------------------

add_executable(mina ${MINA_EXE_SRC} ${MINA_ENGINE_SRC})
target_compile_options(mina PUBLIC ${MINA_CXX_FLAGS} ${MINA_CXX_SAN_FLAGS})
target_link_libraries(mina PUBLIC ${MINA_CXX_SAN_FLAGS} Vulkan::Vulkan glfw)
target_include_directories(mina PUBLIC "${CMAKE_SOURCE_DIR}/include")

add_library(mina_lib ${MINA_ENGINE_SRC})
target_compile_options(mina_lib PUBLIC ${MINA_CXX_FLAGS} ${MINA_CXX_SAN_FLAGS})
target_link_libraries(mina_lib PUBLIC ${MINA_CXX_SAN_FLAGS} Vulkan::Vulkan glfw)
target_include_directories(mina_lib PUBLIC "${CMAKE_SOURCE_DIR}/include")

# ------------------------------------------------------------------------------
# Mina library tests
# ------------------------------------------------------------------------------

list(
    APPEND TESTS
        "test_memory_map"
)

foreach(t IN LISTS TESTS)
    add_executable(${t} "${CMAKE_SOURCE_DIR}/test/${t}.cpp")
    target_compile_options(${t} PRIVATE ${COMMON_CXX_FLAGS})
    target_link_libraries(${t} PUBLIC mina_lib)
endforeach()
