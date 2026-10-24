// SPDX-License-Identifier: GPL-2.0
// Native property projection is separate from tree walking and extraction.
#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Windows/PropVariant.h"
#include "Windows/TimeUtils.h"
#include "btrfshandler.h"
#include "include/linux/kdev_t.h"
#include <algorithm>
#include <cstring>
namespace NArchive::NBtrfs {
namespace {
static bool inline IsEmptyData(const Byte *data, unsigned size) {
    for (unsigned i = 0; i < size; i++)
        if (data[i] != 0)
            return false;
    return true;
}
} // namespace
Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (index >= items_.size())
        return E_INVALIDARG;
    const auto &item = items_[index];
    const auto &inode = item.inode;
    NWindows::NCOM::CPropVariant prop;
    switch (id) {
    case kpidPath:
        prop = item.path;
        break;
    case kpidName:
        prop = item.attribute ? item.attribute->name.Ptr()
                              : item.path.Ptr(item.path.ReverseFind_PathSepar() + 1);
        break;
    case kpidIsAltStream:
        prop = bool(item.attribute);
        break;
    case kpidIsDir:
        prop = IsDir(inode);
        break;
    case kpidSize:
        if (!IsDir(inode))
            prop = UInt64(HasData(inode) ? inode.size : 0);
        break;
    case kpidPackSize:
        if (!IsDir(inode))
            prop = UInt64(HasData(inode) ? inode.packed_size : 0);
        break;
    case kpidPosixAttrib:
        prop = UInt32(inode.mode);
        break;
    case kpidUserId:
        prop = UInt32(inode.uid);
        break;
    case kpidGroupId:
        prop = UInt32(inode.gid);
        break;
    case kpidINode:
        if (!item.attribute)
            prop = UInt64(inode.ino);
        break;
    case kpidStreamId:
        if (item.attribute)
            prop = (UInt64(1) << 63) | index;
        else if (!IsDir(inode))
            prop = UInt64(inode.root);
        break;
    case kpidLinks:
        prop = UInt32(inode.nlink);
        break;
    case kpidSymLink:
        if (!item.symlink.IsEmpty())
            prop = item.symlink;
        break;
    case kpidDeviceMajor:
    case kpidDeviceMinor:
        if ((inode.mode & typeMask) == 0020000 || (inode.mode & typeMask) == 0060000)
            prop = UInt32(id == kpidDeviceMajor ? MAJOR(inode.rdev) : MINOR(inode.rdev));
        break;
    case kpidMTime:
    case kpidATime:
    case kpidCTime:
    case kpidChangeTime: {
        const Int64 seconds = id == kpidMTime   ? inode.mtime
                              : id == kpidATime ? inode.atime
                              : id == kpidCTime ? inode.otime
                                                : inode.ctime;
        const UInt32 nanos = id == kpidMTime   ? inode.nsec
                             : id == kpidATime ? inode.atime_nsec
                             : id == kpidCTime ? inode.otime_nsec
                                               : inode.ctime_nsec;
        UInt64 ticks;
        if (nanos < 1000000000 && NWindows::NTime::UnixTime64_To_FileTime64(seconds, ticks) &&
            ticks <= UINT64_MAX - nanos / 100) {
            ticks += nanos / 100;
            FILETIME time{UInt32(ticks), UInt32(ticks >> 32)};
            prop = time;
        }
        break;
    }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}
Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID id, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidError && !openError_.IsEmpty())
        prop = openError_;
    else if (id == kpidErrorFlags && !openError_.IsEmpty())
        prop = UInt32(kpv_ErrorFlags_UnsupportedFeature);
    else if (reader_) {
        const auto &info = reader_->info;
        switch (id) {
        case kpidId: {
            if (!IsEmptyData(info.uuid, sizeof(info.uuid))) {
                char text[sizeof(info.uuid) * 2 + 2];
                ConvertDataToHex_Lower(text, info.uuid, sizeof(info.uuid));
                prop = text;
            }
            break;
        }
        case kpidWarning:
            if (info.log_pending)
                prop = L"Btrfs is read without tree-log replay; the latest logged changes may be "
                       L"absent.";
            break;
        case kpidPhySize:
            prop = reader_->physicalSize;
            break;
        case kpidTotalSize:
            prop = UInt64(info.size);
            break;
        case kpidNumVolumes:
            prop = UInt64(info.devices);
            break;
        case kpidVolumeName:
            prop = Text(info.label, strlen(info.label));
            break;
        case kpidClusterSize:
            prop = UInt32(info.sectorsize);
            break;
        case kpidINode:
            prop = true;
            break;
        case kpidIsTree:
            prop = true;
            break;
        case kpidIsAltStream:
            prop = std::any_of(items_.cbegin(), items_.cend(),
                               [](const Item &item) { return bool(item.attribute); });
            break;
        }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

static const Byte kProps[] = {
    kpidPath,        kpidIsAltStream, kpidIsDir,   kpidSize,  kpidPackSize,  kpidPosixAttrib,
    kpidMTime,       kpidUserId,      kpidGroupId, kpidINode, kpidLinks,     kpidSymLink,
    kpidDeviceMajor, kpidDeviceMinor, kpidATime,   kpidCTime, kpidChangeTime};
static const Byte kArcProps[] = {kpidVolumeName, kpidClusterSize, kpidId};

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *type)) {
    if (index >= items_.size())
        return E_INVALIDARG;
    *parent = items_[index].parent;
    *type = items_[index].attribute ? NParentType::kAltStream : NParentType::kDir;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32, BSTR *name, PROPID *id)) {
    *name = nullptr;
    *id = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawProp(UInt32, PROPID, const void **data, UInt32 *size, UInt32 *type)) {
    *data = nullptr;
    *size = 0;
    *type = 0;
    return S_OK;
}
IMP_IInArchive_Props IMP_IInArchive_ArcProps
} // namespace NArchive::NBtrfs
