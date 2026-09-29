// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Bound source IFolder owns metadata and paths; Qt supplies list controls and
// narrow presentation/operation adapters around the partial source panel.
#pragma once

#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "Common/MyTypes.h"
#include "Common/MyVector.h"
#include "qtpanellist.h"
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
    bool PackSizeDefined = true;
    UInt64 NumSubDirs = 0;
    UInt64 NumSubFiles = 0;
    CObjectVector<CPanelProperty> Properties;
    // Unparsed 7zz timestamp compatibility text; native FILETIME parsing remains P0-03/P0-04.
    QString MTimeText;
};

struct CSelectedState {
    int FocusedItem;
    bool SelectFocused;
    bool FocusedName_Defined;
    bool CalledFromTimer;
    UString FocusedName;
    UStringVector SelectedNames;

    CSelectedState()
        : FocusedItem(-1), SelectFocused(true), FocusedName_Defined(false), CalledFromTimer(false) {
    }
};

class CPanel : public QObject {
  public:
    CPanel() = default;
    CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items);
    explicit CPanel(IFolderFolder *folder);
    void ReleaseFolder();
    void SetNewFolder(IFolderFolder *folder);
    // Borrowed interface for Qt property rendering; ownership stays in CPanel.
    IFolderFolder *GetFolder() const { return _folder; }
    void AttachPresentation(QWidget *list, QWidget *address);
    void SetFocusToList();
    void SetFocusToLastRememberedItem();
    void AttachSelectionView(QAbstractItemView *view) { _selectionList.Attach(view); }
    void SaveSelectedState(CSelectedState &state);
    void GetSelectedNames(UStringVector &selectedNames);
    void Get_ItemIndices_Selected(CRecordVector<UInt32> &indices) const;
    void Get_ItemIndices_Operated(CRecordVector<UInt32> &indices) const;
    // Qt supplies its display prefix until source LoadFullPath/parent-stack integration.
    void SetCurrentFolderPrefix(const QString &prefix) { _currentFolderPrefix = ToUString(prefix); }
    UString GetFolderTypeID() const;
    bool IsFolderTypeEqTo(const char *s) const;
    bool IsRootFolder() const;
    bool IsFSFolder() const;
    bool IsFSDrivesFolder() const;
    bool IsAltStreamsFolder() const;
    bool IsArcFolder() const;
    bool IsHashFolder() const;
    bool IsDeviceDrivesPrefix() const { return _currentFolderPrefix.IsEqualTo("\\\\.\\"); }
    bool IsSuperDrivesPrefix() const { return _currentFolderPrefix.IsEqualTo("\\\\?\\"); }
    UString GetFsPath() const;
    UString GetItemRelPath(unsigned index) const;
    UString GetItemRelPath2(unsigned index) const;
    UString GetItemFullPath(unsigned index) const;
    bool IsItem_Folder(UInt32 index) const;
    // Partial RefreshListCtrl: rows are loaded/sorted by the presentation owner.
    void RefreshListCtrl(const CSelectedState &state);
    bool _lastFocusedIsList = true;
    QString GetItemsInfoString(const CRecordVector<UInt32> &indices) const;
    void Properties(const CRecordVector<UInt32> &operatedIndices, CListViewDialog &message) const;
    static QString FoldersHistory(CFolderHistory &history, const QString &settingsPath,
                                  QWidget *parent);
    static void SetBookmark(CFastFolders &fastFolders, unsigned index,
                            const UString &currentFolderPrefix);
    static UString OpenBookmark(CFastFolders &fastFolders, unsigned index);

  private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    QPointer<QWidget> _listView;
    QPointer<QWidget> _headerComboBox;
    QtPanelList _selectionList;
    CRecordVector<bool> _selectedStatusVector;
    CMyComPtr<IFolderFolder> _folder;
    CMyComPtr<IFolderGetItemName> _folderGetItemName;
    CRecordVector<bool> _isDirVector;
    UString _currentFolderPrefix;
    bool GetItem_BoolProp(UInt32 index, PROPID propID) const;
    UString GetItemName(unsigned index) const;
    UString GetItemPrefix(unsigned index) const;
    unsigned GetRealItemIndex(int indexInListView) const;
    UInt32 GetFolderItemCount() const;
    void SetFocusedSelectedItem(int index, bool select);
    UInt64 GetItemSize(UInt32 index) const;
    UInt64 GetItem_UInt64Prop(UInt32 index, UInt32 propID) const;
};

QString ConvertSizeToString(UInt64 value);
bool IsSizeProp(UINT propID) noexcept;
void AddValuePair2(QString &s, const QString &label, UInt64 num, UInt64 size);
UString GetFolderPath(IFolderFolder *folder);
