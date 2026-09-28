// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt supplies a snapshot of IFolder properties for selection summaries and
// property lists; panel controls and operations remain in their adapters.
#pragma once

#include "Common/MyString.h"
#include "Common/MyTypes.h"
#include "Common/MyVector.h"
#include <QObject>
#include <QPointer>
#include <QString>

class CFolderHistory;
class CFastFolders;
class CListViewDialog;
class QWidget;

struct CPanelProperty {
    UString Name;
    UString Value;
};

struct CPanelItem {
    QString RelPath;
    bool IsFolder = false;
    bool SizeDefined = false;
    UInt64 Size = 0;
    UInt64 PackSize = 0;
    UInt64 NumSubDirs = 0;
    UInt64 NumSubFiles = 0;
    CObjectVector<CPanelProperty> Properties;
};

class CPanel : public QObject {
  public:
    CPanel() = default;
    CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items);
    void AttachPresentation(QWidget *list, QWidget *address);
    void SetFocusToList();
    void SetFocusToLastRememberedItem();
    bool _lastFocusedIsList = true;
    QString GetItemsInfoString(const CRecordVector<UInt32> &indices) const;
    void Properties(const CRecordVector<UInt32> &operatedIndices, bool archive,
                    CListViewDialog &message) const;
    static QString FoldersHistory(CFolderHistory &history, const QString &settingsPath,
                                  QWidget *parent);
    static void SetBookmark(CFastFolders &fastFolders, unsigned index,
                            const UString &currentFolderPrefix);
    static UString OpenBookmark(CFastFolders &fastFolders, unsigned index);

  private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    QPointer<QWidget> _listView;
    QPointer<QWidget> _headerComboBox;
    CObjectVector<CPanelItem> _folder;
    QString _currentFolderPrefix;
    bool IsItem_Folder(UInt32 index) const { return _folder[index].IsFolder; }
    QString GetItemRelPath(UInt32 index) const { return _folder[index].RelPath; }
    UInt64 GetItemSize(UInt32 index) const;
    UInt64 GetItem_UInt64Prop(UInt32 index, UInt32 propID) const;
};

QString ConvertSizeToString(UInt64 value);
void AddValuePair2(QString &s, const QString &label, UInt64 num, UInt64 size);
