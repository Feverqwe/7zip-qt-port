// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Only the copy-selection summary is ported here. Qt supplies a snapshot of
// IFolder properties; panel controls and operations remain in their adapters.
#pragma once

#include "Common/MyTypes.h"
#include "Common/MyVector.h"
#include <QString>

class CFolderHistory;
class QWidget;

struct CPanelItem {
    QString RelPath;
    bool IsFolder = false;
    bool SizeDefined = false;
    UInt64 Size = 0;
};

class CPanel {
  public:
    CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items);
    QString GetItemsInfoString(const CRecordVector<UInt32> &indices) const;
    static QString FoldersHistory(CFolderHistory &history, const QString &settingsPath,
                                  QWidget *parent);

  private:
    CObjectVector<CPanelItem> _folder;
    QString _currentFolderPrefix;
    bool IsItem_Folder(UInt32 index) const { return _folder[index].IsFolder; }
    QString GetItemRelPath(UInt32 index) const { return _folder[index].RelPath; }
};

QString ConvertSizeToString(UInt64 value);
void AddValuePair2(QString &s, const QString &label, UInt64 num, UInt64 size);
