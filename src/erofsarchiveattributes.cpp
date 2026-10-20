// SPDX-License-Identifier: GPL-2.0-only
// Owned archive values and filename projection around Linux xattr operations.
#include "erofsarchiveattributes.h"
#include <cerrno>
namespace {
struct Context {
    std::vector<ErofsAttribute> &values;
    HRESULT result = S_OK;
};
int Emit(void *opaque, const char *name, const void *value, size_t size) noexcept {
    auto &context = *static_cast<Context *>(opaque);
    context.result = AppendArchiveAttribute(context.values, name, value, size);
    return context.result == S_OK ? 0 : -EIO;
}
} // namespace
HRESULT ReadErofsAttributes(ErofsArchiveReader &reader, erofs_file *file,
                            std::vector<ErofsAttribute> &values) {
    Context context{values};
    const int result = erofs_read_xattrs(file, &context, Emit);
    return context.result != S_OK ? context.result : reader.Result(result);
}
