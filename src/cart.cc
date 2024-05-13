#include <mina/cart.h>

#include <psh/file_system.h>

namespace mina {
    psh::FileStatus GbCart::init(psh::Arena* arena, psh::StringView path_) noexcept {
        path.init(arena, path_);
        psh::File           cart{arena, path_, "rb"};
        psh::FileReadResult fread = cart.read(arena);
        content                   = fread.content;
        return fread.status;
    }
}  // namespace mina
