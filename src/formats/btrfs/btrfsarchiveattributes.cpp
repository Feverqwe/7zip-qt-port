// SPDX-License-Identifier: GPL-2.0-only
// Owned archive values and filename projection around Linux xattr operations.
#include "formats/btrfs/btrfsarchiveattributes.h"
#include <cerrno>
namespace {
struct Context {
    std::vector<BtrfsAttribute> &values;
    HRESULT result = S_OK;
};
int Emit(void *opaque, const char *name, const void *value, size_t size) noexcept {
    auto &context = *static_cast<Context *>(opaque);
    context.result = AppendArchiveAttribute(context.values, name, value, size);
    return context.result == S_OK ? 0 : -EIO;
}
} // namespace
HRESULT ReadBtrfsAttributes(BtrfsArchiveReader &reader, btrfs_file *file,
                            std::vector<BtrfsAttribute> &values) {
    Context context{values};
    const int result = btrfs_read_xattrs(file, &context, Emit);
    return context.result != S_OK ? context.result : reader.Result(result);
}
