// Adapted from 7-Zip CPP/7zip/UI/FileManager/App.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// GetItemsInfoString and its helpers retain upstream control flow. Qt resource
// strings and a folder-property snapshot replace Win32 language/COM access.
#include "CopyDialog.h"
#include "Panel.h"

CPanel::CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items)
    : _folder(items), _currentFolderPrefix(prefix) {}

static void AddSizeValue(QString &s, UInt64 size) {
    s += QStringLiteral("%1 байт").arg(ConvertSizeToString(size));
}

static void AddValuePair1(QString &s, const QString &label, UInt64 size) {
    s += label;
    s += ": ";
    AddSizeValue(s, size);
    s += '\n';
}

static void AddValuePair2(QString &s, const QString &label, UInt64 num, UInt64 size) {
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

static void AddPropValueToSum(const CObjectVector<CPanelItem> &folder, UInt32 index, UInt64 &sum) {
    if (sum == (UInt64)(Int64)-1)
        return;
    const CPanelItem &prop = folder[index];
    if (prop.SizeDefined)
        sum += prop.Size;
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
            AddPropValueToSum(_folder, index, foldersSize);
            numDirs++;
        } else {
            AddPropValueToSum(_folder, index, filesSize);
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
