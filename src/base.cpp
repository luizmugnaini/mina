#include <mina/base.h>

#include <cstdlib>

namespace mina {
    void abort_program() noexcept {
        mina_unused(std::fprintf(stderr, "Mina: aborting program...\n"));
        std::abort();
    }

    void log(LogInfo&& info, StrPtr msg) noexcept {
#if !defined(MINA_DISABLE_LOGGING)
        mina_unused(std::fprintf(
            stderr,
            "%s [%s:%d] %s\n",
            log_level_str(info.lvl),
            info.file,
            info.line,
            msg));
#endif
    }

    void assert_(bool expr_res, StrPtr expr_str, StrPtr msg, LogInfo&& info) noexcept {
#if !defined(MINA_DISABLE_ASSERTS)
        if (!expr_res) {
            log_fmt(info, "Assertion failed: %s, msg: %s", expr_str, msg);
            abort_program();
        }
#endif
    }

}  // namespace mina
