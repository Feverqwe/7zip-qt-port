// Adapted from 7-Zip CPP/7zip/UI/FileManager/App.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// GetItemsInfoString and its helpers retain upstream control flow. Qt resource
// strings adapt language access; IFolder and variant ownership stay upstream.
#include "7zip/PropID.h"
#include "AppState.h"
#include "CopyDialog.h"
#include "Panel.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "qtpanelfolder.h"

static void AddUniqueStringToHead(UStringVector &list, const UString &s) {
    for (unsigned i = 0; i < list.Size();)
        if (s.IsEqualTo_NoCase(list[i]))
            list.Delete(i);
        else
            i++;
    list.Insert(0, s);
}

void CFolderHistory::Normalize() {
    const unsigned kMaxSize = 100;
    if (Strings.Size() > kMaxSize)
        Strings.DeleteFrom(kMaxSize);
}

void CFolderHistory::AddString(const UString &s) {
    AddUniqueStringToHead(Strings, s);
    Normalize();
}

CPanel::CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items)
    : _currentFolderPrefix(prefix) {
    SetNewFolder(new QtPanelFolder(prefix, items));
}

CPanel::CPanel(IFolderFolder *folder) {
    SetNewFolder(folder);
    if (_folder)
        _currentFolderPrefix = ToQString(GetFolderPath(_folder));
}

static void AddSizeValue(QString &s, UInt64 size) {
    s += QStringLiteral("%1 байт").arg(ConvertSizeToString(size));
}

static void AddValuePair1(QString &s, const QString &label, UInt64 size) {
    s += label;
    s += ": ";
    AddSizeValue(s, size);
    s += '\n';
}

void AddValuePair2(QString &s, const QString &label, UInt64 num, UInt64 size) {
    if (num == 0)
        return;
    s += label;
    s += ": ";
    s += ConvertSizeToString(num);
    if (size != (UInt64)(Int64)-1) {
        s += "    ( ";
        AddSizeValue(s, size);
        s += " )";
    }
    s += '\n';
}

static void AddPropValueToSum(IFolderFolder *folder, UInt32 index, PROPID propID, UInt64 &sum) {
    if (sum == (UInt64)(Int64)-1)
        return;
    NWindows::NCOM::CPropVariant prop;
    folder->GetProperty(index, propID, &prop);
    UInt64 val = 0;
    if (ConvertPropVariantToUInt64(prop, val))
        sum += val;
    else
        sum = (UInt64)(Int64)-1;
}

QString CPanel::GetItemsInfoString(const CRecordVector<UInt32> &indices) const {
    QString info;
    UInt64 numDirs, numFiles, filesSize, foldersSize;
    numDirs = numFiles = filesSize = foldersSize = 0;
    unsigned i;
    for (i = 0; i < indices.Size(); i++) {
        const UInt32 index = indices[i];
        if (IsItem_Folder(index)) {
            AddPropValueToSum(_folder, index, kpidSize, foldersSize);
            numDirs++;
        } else {
            AddPropValueToSum(_folder, index, kpidSize, filesSize);
            numFiles++;
        }
    }
    AddValuePair2(info, QStringLiteral("Папок"), numDirs, foldersSize);
    AddValuePair2(info, QStringLiteral("Файлов"), numFiles, filesSize);
    int numDefined = ((foldersSize != (UInt64)(Int64)-1) && foldersSize != 0) ? 1 : 0;
    numDefined += ((filesSize != (UInt64)(Int64)-1) && filesSize != 0) ? 1 : 0;
    if (numDefined == 2)
        AddValuePair1(info, QStringLiteral("Размер"), filesSize + foldersSize);
    info += '\n';
    info += _currentFolderPrefix;
    for (i = 0; i < indices.Size() && (int)i < (int)kCopyDialog_NumInfoLines - 6; i++) {
        info += '\n';
        info += "  ";
        const UInt32 index = indices[i];
        info += GetItemRelPath(index);
        if (IsItem_Folder(index))
            info += '/';
    }
    if (i != indices.Size()) {
        info += '\n';
        info += "  ...";
    }
    return info;
}
