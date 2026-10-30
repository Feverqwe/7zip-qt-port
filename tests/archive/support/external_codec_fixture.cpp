// Test-only registration of the original Copy coder under a distinct ID.
// The algorithm and module ABI/export code are unchanged upstream sources.
#include "7zip/Common/RegisterCodec.h"
#include "7zip/Compress/CopyCoder.h"

namespace NCompress {
REGISTER_CODEC_CREATE(CreateFixtureCopy, CCopyCoder())
REGISTER_CODEC_2(FixtureCopy, CreateFixtureCopy, CreateFixtureCopy, 0xF001, "SevenzipFixtureCopy")
} // namespace NCompress
