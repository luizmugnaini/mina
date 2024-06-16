#include <mina/cartridge.h>

#include <psh/file_system.h>

namespace mina {
    psh::FileStatus GbCart::init(psh::Arena* arena, psh::StringView path_) noexcept {
        path.init(arena, path_);
        psh::File           cart{arena, path_, psh::OpenFileFlag::READ_BIN};
        psh::FileReadResult fread = psh::read_file(arena, cart);
        content                   = fread.content;
        return fread.status;
    }
}  // namespace mina
