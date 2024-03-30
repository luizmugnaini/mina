#!/bin/python3
"""
Helper script for dealing with the codebase. Use `python mk --help` for more info.

Author: Luiz G. Mugnaini A. <luizmugnaini@gmail.com>
"""
from enum import Enum, auto
import os
import sys
import glob
import subprocess as sp
from pathlib import Path

HELP = """Python script for a better experience with CMake. 

\x1b[1;4mUsage\x1b[24m: ./maker\x1b[0m [OPTIONS] [COMMAND] [ARGS]

\x1b[1;4mCommands\x1b[0m:
    \x1b[1mbuild\x1b[0m             Build all targets.
    \x1b[1mbuild-tidy\x1b[0m        Build with clang-tidy enabled.
    \x1b[1mrun\x1b[0m
        \x1b[1mrun list\x1b[0m      Lists all binaries from `./build/bin/`.
        \x1b[1mrun\x1b[0m [ARG]     Run the binary [ARG] from `./build/bin/`.
    \x1b[1mtest\x1b[0m
        \x1b[1mtest\x1b[0m          Runs all binaries starting with "test" in `./build/bin/`.
        \x1b[1mtest\x1b[0m list     Lists all binaries starting with "test" in `./build/bin/`.
        \x1b[1mtest\x1b[0m [ARG]    Runs the test that matches [ARG] in `./build/bin/`. If not
                      found, runs all tests that match the regex "test*{ARG}"
                      in `./build/bin/`.
    \x1b[1mspell\x1b[0m             Spell-check source code and overwrite the files with the corrections,
                      using codespell.
    \x1b[1mformat\x1b[0m            Format all source files, including ./src, ./include,
                      ./examples, and ./tests [aliases: fmt].
    \x1b[1mclear-cache\x1b[0m       Removes the ./build/CMakeCache.txt file.

\x1b[1;4mOptions\x1b[0m:
    -h, --help      Print help."""

POSIX_CPP_COMPILER = "clang++"
POSIX_C_COMPILER = "clang"
POSIX_LINKER = "mold"

WIN_COMPILER = "clang-cl"
WIN_LINKER = "lld-link"


class Lang(Enum):
    CPP = auto()
    C = auto()


def compiler(lang: Lang) -> str:
    match os.name:
        case "nt":
            return WIN_COMPILER
        case "posix":
            match lang:
                case Lang.CPP:
                    return POSIX_CPP_COMPILER
                case Lang.C:
                    return POSIX_C_COMPILER
        case _:
            assert False, "Unreachable"


def linker() -> str:
    match os.name:
        case "posix":
            return POSIX_LINKER
        case "nt":
            return WIN_LINKER
        case _:
            assert False, "Unreachable"


CMAKE_FLAGS = [
    "-G=Ninja",
    f"-DCMAKE_CXX_COMPILER={compiler(Lang.CPP)}",
    f"-DCMAKE_C_COMPILER={compiler(Lang.C)}",
    f"-DCMAKE_EXE_LINKER_FLAGS='-fuse-ld={linker()}'",
    f"-DCMAKE_SHARED_LINKER_FLAGS='-fuse-ld={linker()}'",
]

DEBUG_FLAGS = ["-DCMAKE_BUILD_TYPE=Debug", "-DMINA_DEBUG=On"]
RELEASE_FLAGS = [
    "-DMINA_DISABLE_ASSERTS=On",
    "-DMINA_DISABLE_LOGGINGS=On",
    "-DMINA_DEBUG=Off",
]

TIDY_FLAG = "-DUSE_TIDY=ON"

CPP_SRC_EXTENSION = ".cpp"
CPP_HEADER_EXTENSION = ".h"
BINARY_EXT = ".exe" if os.name == "nt" else ""

SRC_DIR = "./src"
INCLUDE_DIR = "./include"
TEST_DIR = "./tests"
BUILD_DIR = Path("./build")
BIN_DIR = Path(f"{BUILD_DIR}/bin")

DEPENDENCIES = [
    ("glfw", "https://github.com/glfw/glfw.git"),
]

EMU_BIN_NAME = "mina"


def header(action: str):
    """Header used to indicate a action ran by the `./maker` script."""
    action = " " + action + " "
    print(f"\x1b[1;35m{action:-^80}\x1b[0m")


def subaction(subact: str):
    """Indicates that some subaction is being ran by the `./maker` script."""
    indicator = "\x1b[1;32m::\x1b[0m"
    print(f"{indicator} {subact}")


def log_error(msg: str):
    """Logs a message error to the console with an ERROR header."""
    print("\x1b[1;31m[ERROR]\x1b[0m", msg)


def sp_run(cmd: list[str], stderr=sp.STDOUT, **kwargs):
    """Run a subprocess."""
    print("\x1b[1;36m[maker]\x1b[0m", sp.list2cmdline(cmd))
    sp.run(cmd, stderr=stderr, **kwargs)


def get_all_sources() -> list[str]:
    def sources_in(root):
        files = []
        for dir, _, filenames in os.walk(root):
            files += [dir + "/" + f for f in filenames]
        return files

    return sources_in(SRC_DIR) + sources_in(INCLUDE_DIR) + sources_in(TEST_DIR)


def run_bin(bin_path: str | Path):
    """Given a path to a binary, runs the binary."""
    if os.access(bin_path, os.X_OK):
        sp_run([str(bin_path)])
    else:
        log_error(f"File {bin_path} is not executable...")


def list_bin_files(pattern: str | None = None):
    """
    Lists all binary files that match with a pattern (if any was given, otherwise lists all
    binaries).
    """
    search = f"/*{BINARY_EXT}" if pattern is None else f"/*{pattern}*{BINARY_EXT}"
    bin_files = glob.glob(str(BIN_DIR) + search)
    bin_files.sort()
    for bin_file in bin_files:
        print(Path(bin_file).name)


def command_run():
    """Handles the `./maker run` command."""
    if not len(sys.argv) > 2:
        log_error(
            "Incomplete command, please run `./maker --help` for more information."
        )
    elif sys.argv[2] == "list":
        list_bin_files()
    else:
        bin_list = sys.argv[2:]
        build_projects()
        for bin in bin_list:
            bin_path = str(BIN_DIR / bin) + BINARY_EXT
            if os.path.exists(bin_path):
                subaction("Running " + bin_path)
                run_bin(bin_path)
            else:
                log_error(f"Binary {bin_path} not found...")


def run_tests(pattern: str | None = None):
    """
    If given a pattern, runs all tests that match with the pattern. If no pattern is passed, runs
    all tests.
    """
    header("Running tests")
    search = f"test*{BINARY_EXT}" if pattern is None else f"test*{pattern}*{BINARY_EXT}"
    tests = glob.glob(str(BIN_DIR / search))
    tests.sort()
    n_tests = len(tests)

    for idx, test in enumerate(tests):
        subaction(f"[test {idx + 1}/{n_tests}]: {test}...")
        run_bin(test)


def command_test():
    """Handles the `./maker test` command."""
    if not len(sys.argv) > 2:
        run_tests()
    elif sys.argv[2] == "list":
        list_bin_files("test")
    else:
        # On an exact match, we run only that test, otherwise run all tests that match the given
        # string pattern.
        pattern = sys.argv[2]
        if os.path.exists(BIN_DIR / pattern):
            run_bin(pattern)
        else:
            run_tests(pattern)


def command_clear_cache():
    header("Cleaning CMake cache")
    cache_path = "./build/CMakeCache.txt"
    match os.name:
        case "posix":
            sp_run(["rm", "-r", cache_path])
        case "nt":
            sp_run(["cmd", "/c", "del", cache_path])


def command_spell():
    header("Checking for spelling typos")
    sp_run(["codespell"])


def command_format():
    header("Formatting all source and header files")
    sp_run(["clang-format", "-i", *get_all_sources()])


def clear_build():
    if os.path.exists("./build"):
        subaction("Removing existing build directory")
        match os.name:
            case "posix":
                sp_run(["rm", "-rf", "build"])
            case "nt":
                sp_run(["cmd", "/c", "rd", "/s", "/q", "build"])


def resolve_thirdparty():
    header("Cloning dependencies into ./thirdparty")
    for d in DEPENDENCIES:
        path = f"thirdparty/{d[0]}"
        if not os.path.exists(path):
            subaction(f"Cloning {d[0]}")
            sp_run(["git", "clone", d[1], path])


def build_projects(build_flags: list[str] = []):
    if not os.path.exists("./build"):
        header("Generating build system")
        resolve_thirdparty()
        sp_run(
            [
                "cmake",
                *CMAKE_FLAGS,
                *build_flags,
                "-S",
                ".",
                "-B",
                str(BUILD_DIR),
            ],
        )

    header("Building projects")
    sp_run(["cmake", "--build", str(BUILD_DIR)])


def run_game():
    header("Running game")
    sp_run([str(BIN_DIR / EMU_BIN_NAME)])


if not len(sys.argv) > 1:
    log_error("Use `./maker --help` for a list of available commands.")
    sys.exit()

match sys.argv[1].lower():
    case "-h" | "--help" | "help":
        print(HELP)
    case "build":
        if len(sys.argv) > 2 and sys.argv[2].lower() == "release":
            build_projects(build_flags=RELEASE_FLAGS)
        else:
            build_projects(build_flags=DEBUG_FLAGS)
    case "tidy-build":
        header("Building with Clang-Tidy")
        build_projects(build_flags=[*DEBUG_FLAGS, TIDY_FLAG])
    case "run":
        build_projects()
        run_game()
    case "test" | "tests":
        build_projects()
        command_test()
    case "format" | "fmt":
        command_format()
    case "spell":
        command_spell()
    case "clear-cache":
        command_clear_cache()
    case _:
        log_error(
            "Command unavailable, please run `./maker --help` for more information."
        )
