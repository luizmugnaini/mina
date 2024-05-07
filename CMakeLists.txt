# ------------------------------------------------------------------------------
# Project description
# ------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.22)

project(
    mina
    VERSION 0.0.1
    LANGUAGES CXX C
)

# ------------------------------------------------------------------------------
# Compiler info
# ------------------------------------------------------------------------------

message(STATUS "C Compiler executable:   ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler executable: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker executable:       ${CMAKE_LINKER}")

# ------------------------------------------------------------------------------
# Codebase compilation options
#
# You can assign to these options via `-D[OPTION NAME]=[On/Off]`.
# ------------------------------------------------------------------------------

option(MINA_DEBUG                           "Enable all debugging resources"       ON)
option(MINA_ENABLE_VULKAN_VALIDATION_LAYERS "Enable Vulkan validation layers"     OFF)

# ------------------------------------------------------------------------------
# Tooling integration
# ------------------------------------------------------------------------------

# Enable Clangd LSP integration with the project
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Resulting compiler and linker flags for the build
# ------------------------------------------------------------------------------

set(MINA_CXX_FLAGS)      # Holds the resulting compiler flags for the build
set(MINA_CXX_SAN_FLAGS)  # Holds the resulting sanitizer flags for the build

# ------------------------------------------------------------------------------
# Compile-time "define" options
# ------------------------------------------------------------------------------

list(
    APPEND MINA_OPTIONS
        MINA_DEBUG
        MINA_ENABLE_VULKAN_VALIDATION_LAYERS
)

if(UNIX)
    set(MINA_DEFINE_PREFIX "-D")
elseif(MSVC)
    set(MINA_DEFINE_PREFIX "/D")
endif()

foreach(opt IN LISTS MINA_OPTIONS)
    if(${opt})
        set(MINA_CXX_FLAGS ${MINA_CXX_FLAGS} "${MINA_DEFINE_PREFIX}${opt}=On")
    endif()
endforeach()

# MSVC says that fopen is unsafe... cringe
if(MSVC)
    set(MINA_CXX_FLAGS ${MINA_CXX_FLAGS} "${MINA_DEFINE_PREFIX}_CRT_SECURE_NO_WARNINGS")
endif()

# ------------------------------------------------------------------------------
# Compiler flags
#
# Here we'll have shared flags in the form `__MINA_COMMON_CXX_FLAGS_[COMPILERS]`
# and the variables with `MINA_[COMPILER]_CXX_FLAGS` will be the final result of
# the list of flags for the corresponding compiler.
# ------------------------------------------------------------------------------

# ---- Unix GCC and Clang ----

set(
    __MINA_COMMON_CXX_FLAGS_GCC_CLANG
    "-std=c++20"
    "-Wall"
    "-Wextra"
    "-pedantic"
    "-Wuninitialized"
    "-Wswitch"
    "-Wcovered-switch-default"
    "-Wshadow"
    "-Wnon-virtual-dtor"
    "-Wold-style-cast"
    "-Wcast-align"
    "-Wunused"
    "-Woverloaded-virtual"
    "-Wpedantic"
    "-Wconversion"
    "-Wsign-conversion"
    "-Wnull-dereference"
    "-Wdouble-promotion"
    "-Wmisleading-indentation"
    "-fno-rtti"
    "-fno-exceptions"
    "-fno-cxx-exceptions"
    "-fcolor-diagnostics"
    "-fno-force-emit-vtables"
    # Don't warn about non-literal format strings.
    # TODO: we should use -Wformat=2
    # "-Wno-format-security"
    # "-Wno-format-nonliteral"

)

# Final compiler flags
set(MINA_GCC_CXX_FLAGS   ${__MINA_COMMON_CXX_FLAGS_GCC_CLANG})
set(MINA_CLANG_CXX_FLAGS ${__MINA_COMMON_CXX_FLAGS_GCC_CLANG})

# ---- MSVC and Clang-cl ----

set(
    __MINA_COMMON_CXX_FLAGS_MSVC_CLANGCL
    "/std:c++20"
    "/fp:except-"   # Disable exceptions.
    "/GR-"          # Disable emission of RTTI data.
    "/GA"           # Optimize for Windows applications.
)
set(
    MINA_COMMON_CXX_DEBUG_FLAGS_MSVC_CLANGCL
    "/Z7"   # Debug info.
    "/Ob0"  # Disable inlining.
    "/Oy-"  # Disable frame pointer optimization.
)
set(
    MINA_COMMON_CXX_RELEASE_FLAGS_MSVC_CLANGCL
    "/Ob2"  # Inline functions as deemed beneficial by the compiler.
)

# Final compiler flags
set(
    MINA_MSVC_CXX_FLAGS ${__MINA_COMMON_CXX_FLAGS_MSVC_CLANGCL}
    "/EHc"   # Extern "C" defaults to nothrow.
)
set(
    MINA_CLANGCL_CXX_FLAGS ${__MINA_COMMON_CXX_FLAGS_MSVC_CLANGCL}
    "-fcolor-diagnostics"
    "-nobuiltininc"

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

# ------------------------------------------------------------------------------
# Sanitizer flags
#
# Here we'll have shared flags in the form `__MINA_COMMON_SAN_CXX_FLAGS_[COMPILERS]`
# and the variables with `MINA_[COMPILER]_CXX_SAN_FLAGS` will be the final result
# of the variables.
# ------------------------------------------------------------------------------

# ---- GCC and Clang ----

set(
    __MINA_COMMON_CXX_SAN_FLAGS_GCC_CLANG
    "-fsanitize=address"
    "-fsanitize=pointer-compare"
    "-fsanitize=pointer-subtract"
    "-fsanitize=shadow-call-stack"
    "-fsanitize=undefined"
    "-fstack-protector-strong"
    "-fsanitize=leak"
)
set(MINA_GCC_CXX_SAN_FLAGS   ${__MINA_COMMON_CXX_SAN_FLAGS_GCC_CLANG})
set(MINA_CLANG_CXX_SAN_FLAGS ${__MINA_COMMON_CXX_SAN_FLAGS_GCC_CLANG})

# ---- MSVC and Clang-cl ----

set(
    __MINA_COMMON_CXX_SAN_FLAGS_MSVC_CLANGCL
    # TODO: add "/fsanitize=address" flag
)
set(MINA_MSVC_CXX_SAN_FLAGS    ${__MINA_COMMON_CXX_SAN_FLAGS_MSVC_CLANGCL})
set(MINA_CLANGCL_CXX_SAN_FLAGS ${__MINA_COMMON_CXX_SAN_FLAGS_MSVC_CLANGCL})

# ------------------------------------------------------------------------------
# Compiler flags setup per compiler
# ------------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GCC")
    set(MINA_CXX_FLAGS ${MINA_GCC_CXX_FLAGS})
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang
    if(UNIX)
        set(MINA_CXX_FLAGS ${MINA_CXX_FLAGS} ${MINA_CLANG_CXX_FLAGS})
    endif(UNIX)

    # Clang-cl
    if(MSVC)
        set(MINA_CXX_FLAGS ${MINA_CXX_FLAGS} ${MINA_CLANGCL_CXX_FLAGS})
    endif(MSVC)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(MINA_CXX_FLAGS ${MINA_MSVC_CXX_FLAGS})
endif()

# Set debug/release flags for both MSVC and Clang-cl
if(MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(MINA_CXX_FLAGS     ${MINA_CXX_FLAGS}     ${MINA_COMMON_CXX_DEBUG_FLAGS_MSVC_CLANGCL})
        set(MINA_CXX_SAN_FLAGS ${MINA_CXX_SAN_FLAGS} ${MINA_CLANGCL_CXX_SAN_FLAGS})
    else()
        set(MINA_CXX_FLAGS ${MINA_CXX_FLAGS} ${MINA_COMMON_CXX_RELEASE_FLAGS_MSVC_CLANG_CL})
    endif()
endif()

# ------------------------------------------------------------------------------
# Inform the resulting compiler and sanitizer flags
# ------------------------------------------------------------------------------

message(STATUS "C++ compiler flags:  ${MINA_CXX_FLAGS}")
message(STATUS "C++ sanitizer flags: ${MINA_CXX_SAN_FLAGS}")

# ------------------------------------------------------------------------------
# Third-party dependencies
# ------------------------------------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

# - Vulkan -

find_package(Vulkan REQUIRED)

# - GLFW -

set(GLFW_BUILD_TESTS   OFF)
set(GLFW_INSTALL       OFF)
set(GLFW_BUILD_DOCS    OFF)
add_subdirectory("${THIRDPARTY_DIR}/glfw")

# - Presheaf -

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PSH_DEBUG ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PSH_DEBUG OFF)
endif()
set(PSH_NAMESPACED_TYPES OFF)
add_subdirectory("${THIRDPARTY_DIR}/presheaf")

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
