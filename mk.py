"""
Python build script for interacting with CMake and other development tools.

Run `python mk.py --help` for further instructions on how to use the script.

Author: Luiz Gustavo Mugnaini Anselmo <luizmugnaini@gmail.com>
"""

import argparse
import shutil
import os
import glob
import subprocess as sp
from pathlib import Path

# =============================================================================
# Settings.
#
# Here you can change all default programs used in the build process.

POSIX_BUILDER = "Ninja"
POSIX_CPP_COMPILER = "clang++"
POSIX_C_COMPILER = "clang"
POSIX_LINKER = "mold"

WIN_BUILDER = "Ninja"
WIN_CPP_COMPILER = "clang-cl"
WIN_C_COMPILER = "clang-cl"
WIN_LINKER = "lld-link"

# =============================================================================


def os_choose(posix: str, win: str):
    return posix if os.name == "posix" else win


CURRENT_BUILDER = os_choose(POSIX_BUILDER, WIN_BUILDER)
CURRENT_CPP_COMPILER = os_choose(POSIX_CPP_COMPILER, WIN_CPP_COMPILER)
CURRENT_C_COMPILER = os_choose(POSIX_C_COMPILER, WIN_C_COMPILER)
CURRENT_LINKER = os_choose(POSIX_LINKER, WIN_LINKER)


CMAKE_FLAGS = [
    f"-G={CURRENT_BUILDER}",
    f"-DCMAKE_CXX_COMPILER={CURRENT_CPP_COMPILER}",
    f"-DCMAKE_C_COMPILER={CURRENT_C_COMPILER}",
]

if CURRENT_LINKER == "mold":
    CMAKE_FLAGS.extend(
        [
            f"-DCMAKE_EXE_LINKER_FLAGS='-fuse-ld={CURRENT_LINKER}'",
            f"-DCMAKE_SHARED_LINKER_FLAGS='-fuse-ld={CURRENT_LINKER}'",
        ]
    )


DEBUG_FLAGS = ["-DCMAKE_BUILD_TYPE=Debug", "-DGAME_DEBUG=On"]
RELEASE_FLAGS = ["-DCMAKE_BUILD_TYPE=Release", "-DGAME_DEBUG=Off"]

CPP_SRC_EXTENSION = ".cpp"
CPP_HEADER_EXTENSION = ".h"
BINARY_EXT = os_choose("", ".exe")

SRC_DIR = Path("./src")
INCLUDE_DIR = Path("./include")
TEST_DIR = Path("./tests")
BUILD_DIR = Path("./build")
BIN_DIR = BUILD_DIR / "bin"
CMAKE_CACHE_PATH = BUILD_DIR / "CMakeCache.txt"

DEPENDENCIES = [
    {"name": "glfw", "url": "https://github.com/glfw/glfw.git"},
    {"name": "presheaf", "url": "https://github.com/luizmugnaini/presheaf.git"},
]

MINA_BIN_NAME = "mina"

SCRIPT_INDICATOR = "\x1b[1;35m[mk]\x1b[0m"


def header(action: str):
    action = f" {action} "
    print(f"\x1b[1;35m{action:-^80}\x1b[0m")


def log_info(subact: str):
    indicator = "\x1b[1;32m[INFO]\x1b[0m"
    print(f"{SCRIPT_INDICATOR}{indicator} {subact}")


def log_error(msg: str):
    indicator = "\x1b[1;31m[ERROR]\x1b[0m"
    print(f"{SCRIPT_INDICATOR}{indicator}", msg)


def log_command(cmd: str):
    indicator = "\x1b[1;36m[RUN]\x1b[0m"
    print(f"{SCRIPT_INDICATOR}{indicator}", cmd)


def sp_run(cmd: list[str], stderr=sp.STDOUT, **kwargs):
    log_command(sp.list2cmdline(cmd))
    sp.run(cmd, stderr=stderr, **kwargs)


def run_bin(bin_path: str | Path):
    sp_run([str(bin_path)])


def run_tests(pattern: str | None = None):
    header("Running tests")
    search = f"test*{BINARY_EXT}" if pattern is None else f"test*{pattern}*{BINARY_EXT}"
    tests = glob.glob(str(BIN_DIR / search))
    tests.sort()
    n_tests = len(tests)

    for idx, test in enumerate(tests):
        log_info(f"\n\x1b[1;33m[test {idx + 1}/{n_tests}]\x1b[0m: {test}...")
        run_bin(test)


def command_format():
    def get_all_sources() -> list[str]:
        def sources_in(root):
            files = []
            for dir, _, filenames in os.walk(root):
                files += [dir + "/" + f for f in filenames]
            return files

        return sources_in(SRC_DIR) + sources_in(INCLUDE_DIR) + sources_in(TEST_DIR)

    header("Formatting all source and header files")
    sp_run(["clang-format", "-i", *get_all_sources()])


def command_spell():
    header("Checking for spelling typos")
    sp_run(["codespell"])


def command_clear_cache():
    header("Cleaning CMake cache")
    os.remove(CMAKE_CACHE_PATH)


def command_clean():
    header("Removing existing build directory")
    try:
        shutil.rmtree("./build")

    except OSError as e:
        log_error(
            f"Unable to remove build directory due to error: [{e.filename}] {e.strerror}"
        )
        return


def resolve_thirdparty():
    for d in DEPENDENCIES:
        dep_name = d["name"]
        dep_url = d["url"]
        path = f"thirdparty/{dep_name}"
        if not os.path.exists(path):
            log_info(f"Cloning {dep_name} into {path}")
            sp_run(["git", "clone", dep_url, path])
        else:
            log_info(f"Dependency {dep_name} already satisfied")


def command_build(build_flags: list[str] = []):
    if not os.path.exists("./build"):
        header("Resolving third-party dependencies")
        resolve_thirdparty()

        header("Generating the build directory")
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

    header("Building project")
    sp_run(["cmake", "--build", str(BUILD_DIR)])


def command_run_mina():
    command_build()

    header("Running game")
    sp_run([str(BIN_DIR / MINA_BIN_NAME)])


def command_test(pattern: str):
    command_build()
    if pattern == "all":
        run_tests()
    else:
        # On an exact match, we run only that test, otherwise run all tests that match the given
        # string pattern.
        if os.path.exists(BIN_DIR / pattern):
            run_bin(pattern)
        else:
            run_tests(pattern)


parser = argparse.ArgumentParser(
    prog="mk", description="Python script for a better experience with CMake."
)

parser.add_argument(
    "--c-compiler", default=None, help="Specify the C compiler to be used"
)
parser.add_argument(
    "--cpp-compiler", default=None, help="Specify the C++ compiler to be used"
)
parser.add_argument("--linker", default=None, help="Specify the linker to be used")
parser.add_argument(
    "--builder",
    default=None,
    help="Specify the builder to be used, like Ninja, Make, etc",
)
parser.add_argument("--clean", action="store_true", help="Remove the build directory")
parser.add_argument(
    "-b",
    "--build",
    nargs="?",
    choices=["debug", "release"],
    const="debug",
    default=None,
    help="Build the project (default: %(default)s)",
)
parser.add_argument(
    "-r", "--run", action="store_true", help="Build and run the project binary"
)
parser.add_argument(
    "-t",
    "--test",
    nargs="?",
    const="all",
    default=None,
    help="Build and run all test of the project",
)
parser.add_argument(
    "--clear-cache",
    action="store_true",
    help="Clear the cache stored by CMake",
)
parser.add_argument(
    "--fmt", action="store_true", help="Format all source files with Clang-Format"
)
parser.add_argument(
    "--spell", action="store_true", help="Spell-check all source files with codespell"
)

args = parser.parse_args()

if args.builder is not None:
    CURRENT_BUILDER = args.builder
if args.c_compiler is not None:
    CURRENT_C_COMPILER = args.c_compiler
if args.cpp_compiler is not None:
    CURRENT_CPP_COMPILER = args.cpp_compiler
if args.linker is not None:
    CURRENT_LINKER = args.linker

if args.fmt:
    command_format()
if args.spell:
    command_spell()
if args.clean:
    command_clean()
if args.clear_cache:
    command_clear_cache()
if args.build is not None:
    match args.build:
        case "debug":
            command_build(build_flags=DEBUG_FLAGS)
        case "release":
            command_build(build_flags=RELEASE_FLAGS)
if args.run:
    command_run_mina()
if args.test is not None:
    command_test(args.test)
