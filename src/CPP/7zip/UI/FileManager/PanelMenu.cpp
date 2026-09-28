// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelMenu.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Properties single-item enumeration and multi-item totals retain upstream
// control flow. A snapshot replaces IFolder/COM. Raw/archive-level properties
// and Windows shell invocation are outside this partial port.
#include "7zip/PropID.h"
#include "Common/IntToString.h"
#include "ListViewDialog.h"
#include "Panel.h"
#include "ViewSettings.h"

static const char *const kSeparator = "------------------------";

static void AddListAscii(CListViewDialog &dialog, const char *s) {
    dialog.Strings.Add((UString)s);
    dialog.Values.AddNew();
}

static void AddSeparator(CListViewDialog &dialog) { AddListAscii(dialog, kSeparator); }

static void AddPropertyPair(const UString &name, const UString &val, CListViewDialog &dialog) {
    dialog.Strings.Add(name);
    dialog.Values.Add(val);
}

static void AddPropertyString(const CPanelProperty &prop, CListViewDialog &dialog) {
    if (!prop.Value.IsEmpty())
        AddPropertyPair(prop.Name, prop.Value, dialog);
}

static void AddPropertyString(UInt32 propID, UInt64 val, CListViewDialog &dialog) {
    const char *name = "";
    switch (propID) {
    case kpidNumSubDirs:
        name = "Папок";
        break;
    case kpidNumSubFiles:
        name = "Файлов";
        break;
    case kpidSize:
        name = "Размер";
        break;
    case kpidPackSize:
        name = "Сжатый размер";
        break;
    }
    UString value;
    if (propID == kpidSize || propID == kpidPackSize)
        value = ToUString(ConvertSizeToString(val));
    else {
        wchar_t temp[32];
        ConvertUInt64ToString(val, temp);
        value = temp;
    }
    AddPropertyPair(ToUString(QString::fromUtf8(name)), value, dialog);
}

void CPanel::Properties(const CRecordVector<UInt32> &operatedIndices, bool archive,
                        CListViewDialog &message) const {
    if (operatedIndices.Size() == 1) {
        UInt32 index = operatedIndices[0];
        const auto &properties = _folder[index].Properties;
        const UInt32 numProps = properties.Size();
        for (UInt32 i = 0; i < numProps; i++)
            AddPropertyString(properties[i], message);
        AddSeparator(message);
    } else if (operatedIndices.Size() >= 1) {
        UInt64 packSize = 0;
        UInt64 unpackSize = 0;
        UInt64 numFiles = 0;
        UInt64 numDirs = 0;
        FOR_VECTOR(i, operatedIndices) {
            const UInt32 index = operatedIndices[i];
            unpackSize += GetItemSize(index);
            packSize += GetItem_UInt64Prop(index, kpidPackSize);
            if (IsItem_Folder(index)) {
                numDirs++;
                numDirs += GetItem_UInt64Prop(index, kpidNumSubDirs);
                numFiles += GetItem_UInt64Prop(index, kpidNumSubFiles);
            } else
                numFiles++;
        }
        AddPropertyPair(
            UString(),
            ToUString(QStringLiteral("Выбрано элементов: %1").arg(operatedIndices.Size())),
            message);
        if (numDirs != 0)
            AddPropertyString(kpidNumSubDirs, numDirs, message);
        if (numFiles != 0)
            AddPropertyString(kpidNumSubFiles, numFiles, message);
        AddPropertyString(kpidSize, unpackSize, message);
        if (archive)
            AddPropertyString(kpidPackSize, packSize, message);
        AddSeparator(message);
    }
    AddPropertyPair(ToUString(QStringLiteral("Путь")), ToUString(_currentFolderPrefix), message);
    message.Title = ToUString(QStringLiteral("Свойства"));
    message.NumColumns = 2;
}
