// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelListNotify.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// The integer and property-to-text branches are copied; QString is the Qt text
// boundary; source time precision is owned by the panel.
#include "7zip/PropID.h"
#include "7zip/UI/Common/PropIDUtils.h"
#include "Panel.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"

#define INT_TO_STR_SPEC(v)                                                                         \
    while (v >= 10) {                                                                              \
        temp[i++] = (Byte)('0' + (unsigned)(v % 10));                                              \
        v /= 10;                                                                                   \
    }                                                                                              \
    *s++ = (Byte)('0' + (unsigned)v);

static void ConvertSizeToString(UInt64 val, wchar_t *s) noexcept {
    Byte temp[32];
    unsigned i = 0;
    if (val <= (UInt32)0xFFFFFFFF) {
        UInt32 val32 = (UInt32)val;
        INT_TO_STR_SPEC(val32)
    } else {
        INT_TO_STR_SPEC(val)
    }
    if (i < 3) {
        if (i != 0) {
            *s++ = temp[(size_t)i - 1];
            if (i == 2)
                *s++ = temp[0];
        }
        *s = 0;
        return;
    }
    unsigned r = i % 3;
    if (r != 0) {
        s[0] = temp[--i];
        if (r == 2)
            s[1] = temp[--i];
        s += r;
    }
    do {
        s[0] = ' ';
        s[1] = temp[(size_t)i - 1];
        s[2] = temp[(size_t)i - 2];
        s[3] = temp[(size_t)i - 3];
        s += 4;
    } while (i -= 3);
    *s = 0;
}

#undef INT_TO_STR_SPEC

QString ConvertSizeToString(UInt64 value) {
    wchar_t s[32];
    ConvertSizeToString(value, s);
    return QString::fromWCharArray(s);
}

bool IsSizeProp(UINT propID) noexcept {
    switch (propID) {
    case kpidSize:
    case kpidPackSize:
    case kpidNumSubDirs:
    case kpidNumSubFiles:
    case kpidOffset:
    case kpidLinks:
    case kpidNumBlocks:
    case kpidNumVolumes:
    case kpidPhySize:
    case kpidHeadersSize:
    case kpidTotalSize:
    case kpidFreeSpace:
    case kpidClusterSize:
    case kpidNumErrors:
    case kpidNumStreams:
    case kpidNumAltStreams:
    case kpidAltStreamsSize:
    case kpidVirtualSize:
    case kpidUnpackSize:
    case kpidTotalPhySize:
    case kpidTailSize:
    case kpidEmbeddedStubSize:
        return true;
    }
    return false;
}

CPanelStatus CPanel::Refresh_StatusBar() const {
    // Preserve PanelListNotify.cpp's four-part calculation. Qt owns the
    // controls, so return the parts instead of calling Win32 SetText.
    CPanelStatus status;
    CRecordVector<UInt32> indices;
    Get_ItemIndices_Operated(indices);

    status.selectedItems =
        QStringLiteral("%1 / %2").arg(indices.Size()).arg(_selectedStatusVector.Size());
    if (!indices.IsEmpty()) {
        UInt64 totalSize = 0;
        FOR_VECTOR(i, indices)
        totalSize += GetItemSize(indices[i]);
        status.selectedSize = ConvertSizeToString(totalSize);
    }

    const int focusedItem = _selectionList.GetFocusedItem();
    if (focusedItem >= 0 && _selectionList.GetSelectedCount() > 0) {
        const unsigned realIndex = GetRealItemIndex(focusedItem);
        if (realIndex != UInt32(-1) && _folder) {
            status.focusedSize = ConvertSizeToString(GetItemSize(realIndex));
            NWindows::NCOM::CPropVariant prop;
            if (_folder->GetProperty(realIndex, kpidMTime, &prop) == S_OK) {
                char dateString[64];
                dateString[0] = 0;
                ConvertPropertyToShortString2(dateString, prop, kpidMTime);
                status.focusedDate = QString::fromLatin1(dateString);
            }
        }
    }
    return status;
}

QString CPanel::GetItemText(UInt32 itemIndex, const CPropColumn &property) const {
    const PROPID propID = property.ID;
    if (!_folder || itemIndex == UInt32(-1))
        return propID == kpidName ? QStringLiteral("..") : QString();
    if (property.IsRawProp) {
        if (!_folderRawProps)
            return {};
        const void *data = nullptr;
        UInt32 dataSize = 0;
        UInt32 propType = 0;
        if (_folderRawProps->GetRawProp(itemIndex, propID, &data, &dataSize, &propType) != S_OK ||
            dataSize == 0 || !data)
            return {};
        if (propID == kpidNtReparse) {
            UString s;
            ConvertNtReparseToString(static_cast<const Byte *>(data), dataSize, s);
            if (!s.IsEmpty())
                return ToQString(s);
        } else if (propID == kpidNtSecure) {
            AString s;
            ConvertNtSecureToString(static_cast<const Byte *>(data), dataSize, s);
            if (!s.IsEmpty())
                return QString::fromLatin1(s.Ptr(), s.Len());
        }
        if (dataSize > 64)
            return QStringLiteral("data:%1").arg(dataSize);
        const char *const hex = dataSize <= 8 && (propID == kpidCRC || propID == kpidChecksum)
                                    ? "0123456789ABCDEF"
                                    : "0123456789abcdef";
        QString value;
        value.reserve(int(dataSize) * 2);
        const Byte *bytes = static_cast<const Byte *>(data);
        for (UInt32 i = 0; i < dataSize; ++i) {
            value.append(QLatin1Char(hex[bytes[i] >> 4]));
            value.append(QLatin1Char(hex[bytes[i] & 15]));
        }
        return value;
    }
    if (propID == kpidName)
        return ToQString(GetItemName(itemIndex));
    if (propID == kpidPrefix)
        return ToQString(GetItemPrefix(itemIndex));
    NWindows::NCOM::CPropVariant prop;
    const HRESULT result = _folder->GetProperty(itemIndex, propID, &prop);
    if (result != S_OK)
        return QStringLiteral("Error: ");
    if ((prop.vt == VT_UI8 || prop.vt == VT_UI4 || prop.vt == VT_UI2) && IsSizeProp(propID)) {
        UInt64 value = 0;
        ConvertPropVariantToUInt64(prop, value);
        return ConvertSizeToString(value);
    }
    if (prop.vt == VT_BSTR) {
        QString value = QString::fromWCharArray(prop.bstrVal);
        value.replace(QLatin1Char('\n'), QLatin1Char(' '));
        value.replace(QLatin1Char('\r'), QLatin1Char(' '));
        return value;
    }
    char text[64];
    ConvertPropertyToShortString2(text, prop, propID, _timestampLevel);
    return QString::fromLatin1(text);
}
