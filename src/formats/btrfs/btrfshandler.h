// SPDX-License-Identifier: GPL-2.0
#ifndef SEVENZIP_BTRFS_HANDLER_H
#define SEVENZIP_BTRFS_HANDLER_H
#include "formats/btrfs/btrfsarchiveattributes.h"
#include <set>
#include <string>
namespace NArchive::NBtrfs {
constexpr UInt32 typeMask = 0170000, directoryType = 0040000, regularType = 0100000,
                 symlinkType = 0120000;
bool IsDir(const btrfs_file_info &);
bool HasData(const btrfs_file_info &);
UString Text(const char *, size_t);
struct Item {
    UString path, symlink;
    btrfs_file_info inode{};
    UInt32 parent = UInt32(-1);
    std::shared_ptr<const BtrfsAttribute> attribute;
};
struct Pending {
    UString prefix;
    BtrfsId nid;
    UInt32 parent = UInt32(-1);
};

class CHandler final : public IInArchive,
                       public IInArchiveGetStream,
                       public IArchiveGetRawProps,
                       public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_3(IInArchive, IInArchiveGetStream, IArchiveGetRawProps)
    Z7_IFACE_COM7_IMP(IInArchive)
    Z7_IFACE_COM7_IMP(IInArchiveGetStream)
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
    std::shared_ptr<BtrfsArchiveReader> reader_;
    std::vector<Item> items_;
    UString openError_;
    struct Tree {
        CHandler &handler;
        const Pending &directory;
        std::vector<Pending> &pending;
        std::set<BtrfsId> &directories;
        std::set<std::string> names;
        HRESULT result = S_OK;
    };
    static int Emit(void *, const char *, size_t, uint64_t, uint64_t, unsigned int);
    HRESULT AddItem(Tree &, const char *, size_t, BtrfsId);
    HRESULT ReadTree();
    HRESULT ReadAttributes();
    HRESULT ExtractItem(UInt32, Int32, IArchiveExtractCallback *);
};

} // namespace NArchive::NBtrfs
#endif
