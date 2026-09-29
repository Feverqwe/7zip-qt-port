// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Bound source IFolder owns metadata and paths; Qt supplies list controls and
// narrow presentation/operation adapters around the partial source panel.
#pragma once

#include "7zip/UI/FileManager/IFolder.h"
#include "CPP/Windows/DLL.h"
#include "CPP/Windows/FileDir.h"
#include "Common/MyBuffer.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "Common/MyTypes.h"
#include "Common/MyVector.h"
#include "Windows/FileFind.h"
#include "Windows/TimeUtils.h"
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

struct CTempFileInfo {
    UInt32 FileIndex; // index of file in folder
    bool NeedDelete;
    UString RelPath; // Relative path of file from Folder
    FString FolderPath;
    FString FilePath;
    NWindows::NFile::NFind::CFileInfo FileInfo;

    CTempFileInfo() : FileIndex((UInt32)(Int32)-1), NeedDelete(false) {}
    void DeleteDirAndFile() const {
        if (NeedDelete) {
            NWindows::NFile::NDir::DeleteFileAlways(FilePath);
            NWindows::NFile::NDir::RemoveDir(FolderPath);
        }
    }
    bool WasChanged_from_TempFileInfo(const NWindows::NFile::NFind::CFileInfo &newFileInfo) const {
        return newFileInfo.Size != FileInfo.Size ||
               Compare_FiTime(&newFileInfo.MTime, &FileInfo.MTime) != 0;
    }
};

struct CFolderLink : public CTempFileInfo {
    bool IsVirtual; // == true (if archive was open via IInStream):
                    //    archive was open from another archive,
                    //    archive size meets the size conditions derived from g_RAM_Size.
                    //    VirtFileSystem was used
                    //    archive was fully extracted to memory.
    bool UsePassword;
    NWindows::NDLL::CLibrary Library;
    CMyComPtr<IFolderFolder>
        ParentFolder;         // can be NULL, if parent is FS folder (in _parentFolders[0])
    UString ParentFolderPath; // including tail slash (doesn't include paths parts of parent in next
                              // level)
    UString Password;
    UString VirtualPath; // without tail slash
    CByteBuffer ZoneBuf; // ZoneBuf for virtaul stream (IsVirtual)

    CFolderLink() : IsVirtual(false), UsePassword(false) {}
    bool WasChanged_from_FolderLink(const NWindows::NFile::NFind::CFileInfo &newFileInfo) const {
        // we call it, if we have two real files.
        // if archive was virtual, it means that we have updated that virtual to real file.
        return IsVirtual || CTempFileInfo::WasChanged_from_TempFileInfo(newFileInfo);
    }
};

class CPanel : public QObject {
  public:
    CPanel() = default;
    CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items);
    explicit CPanel(IFolderFolder *folder);
    // Qt notification adapters replace the Win32 OnNotify entry point.
    bool ShouldProcessNotifications() const { return _processNotify; }
    void Disable_Processing_Timer_Notify_StatusBar() {
        _processTimer = false;
        _processNotify = false;
        _processStatusBar = false;
    }

    class CDisableTimerProcessing {
        Z7_CLASS_NO_COPY(CDisableTimerProcessing)
        bool _processTimer;
        CPanel &_panel;

      public:
        CDisableTimerProcessing(CPanel &panel) : _panel(panel) { Disable(); }
        ~CDisableTimerProcessing() { Restore(); }
        void Disable() {
            _processTimer = _panel._processTimer;
            _panel._processTimer = false;
        }
        void Restore() { _panel._processTimer = _processTimer; }
    };

    class CDisableTimerProcessing2 {
        Z7_CLASS_NO_COPY(CDisableTimerProcessing2)
        bool _processTimer;
        CPanel *_panel;

      public:
        CDisableTimerProcessing2(CPanel *panel) : _processTimer(true), _panel(panel) { Disable(); }
        ~CDisableTimerProcessing2() { Restore(); }
        void Disable() {
            if (_panel) {
                _processTimer = _panel->_processTimer;
                _panel->_processTimer = false;
            }
        }
        void Restore() {
            if (_panel) {
                _panel->_processTimer = _processTimer;
                _panel = NULL;
            }
        }
    };

    class CDisableNotify {
        Z7_CLASS_NO_COPY(CDisableNotify)
        bool _processNotify;
        bool _processStatusBar;
        CPanel &_panel;

      public:
        CDisableNotify(CPanel &panel) : _panel(panel) { Disable(); }
        ~CDisableNotify() { Restore(); }
        void Disable() {
            _processNotify = _panel._processNotify;
            _processStatusBar = _panel._processStatusBar;
            _panel._processNotify = false;
            _panel._processStatusBar = false;
        }
        void SetMemMode_Enable() {
            _processNotify = true;
            _processStatusBar = true;
        }
        void Restore() {
            _panel._processNotify = _processNotify;
            _panel._processStatusBar = _processStatusBar;
        }
    };
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
    // Original Create initializes these to true; Qt panels exist before Create is ported.
    bool _processTimer = true;
    bool _processNotify = true;
    bool _processStatusBar = true;
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
