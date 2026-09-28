// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelItems.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Property snapshot access replaces COM/variant conversion. The optional
// IFolderGetItemName fast path is absent; GetItemSize uses the original tail.
#include "7zip/PropID.h"
#include "Panel.h"

static const UInt32 kParentIndex = (UInt32)-1;

UInt64 CPanel::GetItem_UInt64Prop(UInt32 itemIndex, UInt32 propID) const {
    if (itemIndex == kParentIndex)
        return 0;
    if (itemIndex >= _folder.Size())
        throw 2723400;
    const CPanelItem &item = _folder[itemIndex];
    switch (propID) {
    case kpidSize:
        return item.SizeDefined ? item.Size : 0;
    case kpidPackSize:
        return item.PackSize;
    case kpidNumSubDirs:
        return item.NumSubDirs;
    case kpidNumSubFiles:
        return item.NumSubFiles;
    }
    return 0;
}

UInt64 CPanel::GetItemSize(UInt32 itemIndex) const {
    if (itemIndex == kParentIndex)
        return 0;
    return GetItem_UInt64Prop(itemIndex, kpidSize);
}
