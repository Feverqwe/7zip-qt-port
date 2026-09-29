// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Bound source IFolder owns metadata and paths; Qt supplies list controls and
// narrow presentation/operation adapters around the partial source panel.
#pragma once

#include "7zip/Archive/IArchive.h"
#include "7zip/PropID.h"
#include "7zip/UI/Common/ExtractMode.h"
#include "7zip/UI/FileManager/IFolder.h"
#include "CPP/Windows/DLL.h"
#include "CPP/Windows/FileDir.h"
#include "Common/MyBuffer.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "Common/MyTypes.h"
#include "Common/MyVector.h"
#include "ViewSettings.h"
#include "Windows/FileFind.h"
#include "Windows/Synchronization.h"
#include "Windows/TimeUtils.h"
#include "qtpanellist.h"
#include <QObject>
#include <QPointer>
#include <QString>
#include <functional>
#include <memory>
#include <optional>

class CFolderHistory;
struct CAppState;
class CFastFolders;
class CListViewDialog;
class QWidget;
class CVirtFileSystem;

// Native launcher owner, scoped to this panel rather than a process-wide global.
struct CExitEventLauncher {
    NWindows::NSynchronization::CManualResetEvent _exitEvent;
    bool _needExit;
    unsigned _numActiveThreads;
    CRecordVector<::CThread> _threads;
    CExitEventLauncher() {
        _needExit = false;
        if (_exitEvent.Create(false) != S_OK)
            throw 9387173;
        _needExit = true;
        _numActiveThreads = 0;
    }
    ~CExitEventLauncher() { Exit(true); }
    void Exit(bool hardExit);
};

// Default OS associations can successfully launch without a process handle.
// Known-process/editor services are a separate mandatory transport extension.
class IQtExternalFileServices {
  public:
    virtual ~IQtExternalFileServices() = default;
    virtual HRESULT StartApplication(const UString &dir, const UString &path, QWidget *window) = 0;
};

class CPanelCallback {
  public:
    virtual void OnTab() = 0;
    virtual void SetFocusToPath(unsigned index) = 0;
    virtual void OnCopy(bool move, bool copyToSame) = 0;
    virtual void OnSetSameFolder() = 0;
    virtual void OnSetSubFolder() = 0;
    virtual void PanelWasFocused() = 0;
    virtual void DragBegin() = 0;
    virtual void DragEnd() = 0;
    virtual void RefreshTitle(bool always) = 0;
};

struct CCopyToOptions {
    bool streamMode;
    bool moveMode;
    bool testMode;
    bool includeAltStreams;
    bool replaceAltStreamChars;
    bool showErrorMessages;
    bool NeedRegistryZone;
    NExtract::NZoneIdMode::EEnum ZoneIdMode;
    CByteBuffer ZoneBuf;
    UString folder;
    UStringVector hashMethods;
    CVirtFileSystem *VirtFileSystemSpec;
    CCopyToOptions()
        : streamMode(false), moveMode(false), testMode(false), includeAltStreams(true),
          replaceAltStreamChars(false), showErrorMessages(false), NeedRegistryZone(true),
          ZoneIdMode(NExtract::NZoneIdMode::kNone), VirtFileSystemSpec(NULL) {}
};

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

struct COpenResult {
    bool ArchiveIsOpened;
    bool Encrypted;
    UString ErrorMessage;
    COpenResult() : ArchiveIsOpened(false), Encrypted(false) {}
};

struct CPropColumn {
    int Order;
    PROPID ID;
    VARTYPE Type;
    bool IsVisible;
    bool IsRawProp;
    UInt32 Width;
    UString Name;

    bool IsEqualTo(const CPropColumn &a) const {
        return Order == a.Order && ID == a.ID && Type == a.Type && IsVisible == a.IsVisible &&
               IsRawProp == a.IsRawProp && Width == a.Width && Name == a.Name;
    }
    int Compare(const CPropColumn &a) const { return MyCompare(Order, a.Order); }
    int Compare_NameFirst(const CPropColumn &a) const {
        if (ID == kpidName) {
            if (a.ID != kpidName)
                return -1;
        } else if (a.ID == kpidName)
            return 1;
        return MyCompare(Order, a.Order);
    }
};

Qt::Alignment GetColumnAlign(PROPID propID, VARTYPE varType);

class CPropColumns : public CObjectVector<CPropColumn> {
  public:
    int FindItem_for_PropID(PROPID id) const {
        FOR_VECTOR(i, (*this))
        if ((*this)[i].ID == id)
            return (int)i;
        return -1;
    }
    bool IsEqualTo(const CPropColumns &props) const {
        if (Size() != props.Size())
            return false;
        FOR_VECTOR(i, (*this))
        if (!(*this)[i].IsEqualTo(props[i]))
            return false;
        return true;
    }
};

class CPanel : public QObject {
  public:
    CPanel() = default;
    ~CPanel() override;
    CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items);
    explicit CPanel(IFolderFolder *folder);
    void ReleasePanel();
    void ExitExternalWorkers(bool hardExit);
    HRESULT Create(QWidget *mainWindow, QWidget *parentWindow, UINT id,
                   const UString &currentFolderPrefix, const UString &arcFormat,
                   CPanelCallback *panelCallback, CAppState *appState, bool needOpenArc,
                   COpenResult &openRes);
    bool OnCreate();
    // Replaces CreateEx and Win32 child-control construction, not path binding.
    void AttachCreationPresentation(std::function<bool(CPanel &)> create) {
        _createPresentation = std::move(create);
    }
    bool PanelCreated = false;
    UInt32 _listViewMode = 3;
    int _xSize = 0;
    PROPID _sortID = 0;
    bool _ascending = true;
    bool _mySelectMode = false;
    bool _showDots = false;
    bool _showRealFileIcons = false;
    UInt32 GetListViewMode() const { return _listViewMode; }
    bool GetFlatModeForArc() const { return _flatModeForArc; }
    void SetFlatModeForArc(bool enabled) { _flatModeForArc = enabled; }
    void SetFlatModeForDisk(bool enabled) { _flatModeForDisk = enabled; }
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
    void AttachPanelCallback(CPanelCallback *callback) { _panelCallback = callback; }
    void AttachExternalFileServices(IQtExternalFileServices *services) {
        _externalFileServices = services;
    }
    void RefreshTitle(bool always = false) {
        if (_panelCallback)
            _panelCallback->RefreshTitle(always);
    }
    void RefreshTitleAlways() { RefreshTitle(true); }
    void Get_ZoneId_Stream_from_ParentFolders(CByteBuffer &buf);
    HRESULT CopyTo(CCopyToOptions &options, const CRecordVector<UInt32> &indices,
                   UStringVector *messages, bool &usePassword, UString &password,
                   const UStringVector *filePaths = NULL);
    void SetNewFolder(IFolderFolder *folder);
    void AttachFolderManager(IFolderManager *manager, const QString &settingsPath) {
        _folderManager = manager;
        _settingsPath = settingsPath;
    }
    HRESULT OpenAsArc(IInStream *inStream, const CTempFileInfo &tempFileInfo,
                      const UString &virtualFilePath, const UString &arcFormat,
                      COpenResult &openRes);
    HRESULT OpenAsArc_Msg(IInStream *inStream, const CTempFileInfo &tempFileInfo,
                          const UString &virtualFilePath, const UString &arcFormat);
    HRESULT OpenAsArc_Name(const UString &relPath, const UString &arcFormat);
    HRESULT OpenAsArc_Index(unsigned index, const wchar_t *type = NULL);
    void OpenFolder(unsigned index);
    void OpenFolderExternal(unsigned index);
    void OpenItem(unsigned index, bool tryInternal, bool tryExternal, const wchar_t *type = NULL);
    void OpenFocusedItemAsInternal(const wchar_t *type);
    void OpenSelectedItems(bool tryInternal);
    void OpenItemInArchive(unsigned index, bool tryInternal, bool tryExternal, bool editMode,
                           bool useEditor, const wchar_t *type = NULL);
    bool IsVirus_Message(const UString &name);
    HRESULT OpenParentArchiveFolder();
    HRESULT OnOpenItemChanged(UInt32 index, const wchar_t *fullFilePath, bool usePassword,
                              const UString &password);
    bool OnOpenItemChanged(const CTempFileInfo &tpi, const UString &fullPathFolderPrefix,
                           bool usePassword, const UString &password);
    void CloseOneLevel();
    void CloseOpenFolders();
    void LoadFullPath();
    void SetToRootFolder();
    HRESULT BindToPath(const UString &fullPath, const UString &arcFormat, COpenResult &openRes);
    HRESULT BindToPathAndRefresh(const UString &path);
    UString GetParentDirPrefix() const;
    void OpenParentFolder();
    void OpenRootFolder();
    const CObjectVector<CFolderLink> &GetParentFolders() const { return _parentFolders; }
    const UString &GetCurrentFolderPrefix() const { return _currentFolderPrefix; }
    // Borrowed interface for Qt property rendering; ownership stays in CPanel.
    IFolderFolder *GetFolder() const { return _folder; }
    void AttachPresentation(QWidget *list, QWidget *address);
    void SetFocusToList();
    void SetFocusToPath();
    void SetFocusToLastRememberedItem();
    void AttachSelectionView(QAbstractItemView *view) { _selectionList.Attach(view); }
    void SaveSelectedState(CSelectedState &state);
    void GetSelectedNames(UStringVector &selectedNames);
    void Get_ItemIndices_Selected(CRecordVector<UInt32> &indices) const;
    void Get_ItemIndices_Operated(CRecordVector<UInt32> &indices) const;
    // Fixture-only snapshot prefix; production consumers use LoadFullPath.
    void SetCurrentFolderPrefix(const QString &prefix) { _currentFolderPrefix = ToUString(prefix); }
    UString GetFolderTypeID() const;
    bool IsFolderTypeEqTo(const char *s) const;
    bool IsRootFolder() const;
    bool IsFSFolder() const;
    bool IsFSDrivesFolder() const;
    bool IsAltStreamsFolder() const;
    bool IsArcFolder() const;
    bool IsHashFolder() const;
    bool Is_IO_FS_Folder() const {
        return IsFSFolder() || IsFSDrivesFolder() || IsAltStreamsFolder();
    }
    bool IsFsOrPureDrivesFolder() const {
        return IsFSFolder() || (IsFSDrivesFolder() && !IsDeviceDrivesPrefix());
    }
    bool IsFolder_with_FsItems() const;
    int FindDir_InOperatedList(const CRecordVector<UInt32> &indices) const;
    void CreateFileMenu(class IQtFileMenuServices &menu, bool programMenu);
    bool IsThereReadOnlyFolder() const;
    bool DoesItSupportOperations() const { return _folderOperations != NULL; }
    bool CheckBeforeUpdate(UINT resourceID);
    bool IsDeviceDrivesPrefix() const { return _currentFolderPrefix.IsEqualTo("\\\\.\\"); }
    bool IsSuperDrivesPrefix() const { return _currentFolderPrefix.IsEqualTo("\\\\?\\"); }
    UString GetFsPath() const;
    UString GetItemRelPath(unsigned index) const;
    UString GetItemRelPath2(unsigned index) const;
    UString GetItemFullPath(unsigned index) const;
    bool IsItem_Folder(UInt32 index) const;
    int CompareItemIndices(UInt32 first, UInt32 second, PROPID sortID,
                           bool isRawProp = false) const;
    HRESULT InitColumns();
    void SaveListViewInfo(const CPropColumns &visibleColumns);
    const CPropColumns &GetColumns() const { return _columns; }
    const CPropColumns &GetVisibleColumns() const { return _visibleColumns; }
    QString GetItemText(UInt32 itemIndex, const CPropColumn &property) const;
    bool IsItem_AltStream(unsigned itemIndex) const;
    // Partial RefreshListCtrl: rows are loaded/sorted by the presentation owner.
    void RefreshListCtrl(const CSelectedState &state);
    void RefreshListCtrl();
    // Qt loads rows after synchronous source navigation. Transfer its requested
    // selection to that presentation stage instead of deriving it again there.
    std::optional<CSelectedState> TakeRefreshSelection() {
        auto state = std::move(_refreshSelection);
        _refreshSelection.reset();
        return state;
    }
    bool _lastFocusedIsList = true;
    QString GetItemsInfoString(const CRecordVector<UInt32> &indices) const;
    void Properties(const CRecordVector<UInt32> &operatedIndices, CListViewDialog &message) const;
    static QString FoldersHistory(CFolderHistory &history, const QString &settingsPath,
                                  QWidget *parent);
    static void SetBookmark(CFastFolders &fastFolders, unsigned index,
                            const UString &currentFolderPrefix);
    static UString OpenBookmark(CFastFolders &fastFolders, unsigned index);

  private:
    friend class CApp;
    QPointer<QWidget> _mainWindow;
    QPointer<QWidget> _parentWindow;
    std::function<bool(CPanel &)> _createPresentation;
    CAppState *_appState = nullptr;
    UINT _baseID = 0;
    UINT _comboBoxID = 0;
    UINT _statusBarID = 0;
    CPanelCallback *_panelCallback = nullptr;
    std::optional<CSelectedState> _refreshSelection;
    IQtExternalFileServices *_externalFileServices = nullptr;
    std::unique_ptr<CExitEventLauncher> _exitEventLauncher;
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
    CMyComPtr<IFolderCompare> _folderCompare;
    CMyComPtr<IArchiveGetRawProps> _folderRawProps;
    CMyComPtr<IFolderAltStreams> _folderAltStreams;
    CMyComPtr<IFolderOperations> _folderOperations;
    CObjectVector<CFolderLink> _parentFolders;
    NWindows::NDLL::CLibrary _library;
    bool _flatMode = false;
    bool _flatModeForDisk = false;
    bool _flatModeForArc = false;
    bool _thereAreDeletedItems = false;
    CMyComPtr<IFolderManager> _folderManager;
    QString _settingsPath;
    bool _needSaveInfo = false;
    UString _typeIDString;
    CListViewInfo _listViewInfo;
    CPropColumns _columns;
    CPropColumns _visibleColumns;
    QWidget *GetParent() const;
    void MessageBox_Error(const UString &message) const;
    void MessageBox_Error_LangID(UINT resourceID) const;
    void MessageBox_Error_HRESULT(HRESULT result) const;
    void MessageBox_LastError() const;
    void MessageBox_Error_UnsupportOperation() const;
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
// Source command split used by the Qt external process adapter.
void SplitCmdLineSmart(const UString &cmd, UString &prg, UString &params);
