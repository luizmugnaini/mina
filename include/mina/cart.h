#pragma once

#include <psh/buffer.h>
#include <psh/file_system.h>
#include <psh/intrinsics.h>
#include <psh/string.h>
#include <psh/types.h>

namespace mina {
    struct GbCart {
        psh::String path{};
        psh::String content{};

        psh::FileStatus init(psh::Arena* arena, psh::StringView path_) noexcept;
    };
}  // namespace mina
