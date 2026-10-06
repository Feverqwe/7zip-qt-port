#pragma once
#include <optional>

#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/App.h"
#include "archivebackend.h"
#include "combodialog.h"
#include "copydialog.h"
#include "createarchivedialog.h"
#include "extractdialog.h"
#include "filesystemscanner.h"
#include "filetable.h"
#include "filetransferoperation.h"
#include "qtfolderprojection.h"
#include "settingsdialog.h"
#include "volumeoperations.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringView>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>
#include <array>
#include <functional>
#include <memory>
#include <vector>

class QtFileListHover;
class CProgressDialog;
class IQtProgressTransport;
class IQtTrashService;
struct QtFileManagerStartup;

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(ArchiveBackend *backend = nullptr,
                        std::function<bool(const QUrl &)> fileOpener = {},
                        IQtTrashService *trashService = nullptr,
                        const QtFileManagerStartup *startup = nullptr);

    void openArchive(const QString &path);
    void openPath(const QString &path);

    void extractTo(const QString &destination, bool onlySelected = false,
                   const QString &password = {}, const ExtractionOptions &options = {});

    void testArchive();

  private:
    friend class QtAppPresentation;
    friend class QtFileManagerWindowServices;
    friend class QtFileManagerCommands;
    friend class QtFileManagerDrag;
    friend class QtFileManagerShellMenu;
    friend class QtProgramFileMenu;
    HRESULT openArchiveAs(const QString &path, const QString &type);
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void reloadLangItems();
    void quiesceOperations();
    bool canHandleExternalUpdate() const;
    bool runExternalArchiveUpdate(CPanel &panel, const std::function<bool()> &body);
    QMenu *createMenusAndToolbar();
    void createFileActions(QMenu *menu, QToolBar *toolbar);
    void createFileExtras(QMenu *menu, QToolBar *toolbar);
    void createViewModes(QMenu *viewMenu);
    void createFavoritesMenu(QMenu *favoritesMenu);
    void createViewOptions(QMenu *viewMenu);
    void createToolbarOptions(QMenu *viewMenu);
    void rebuildSourceMenus();
    void activateSingleClickedRow(int row);
    void createFilePanel(QMenu *editMenu, unsigned index);
    void storePanelUi(unsigned index);
    void restorePanelUi(unsigned index);
    void activatePanelUi(unsigned index);
    void withPanelProjection(unsigned index, const std::function<void()> &body);
    unsigned scanningPanelMask() const;
    unsigned panelIndexFor(QObject *object) const;
    void togglePanels();
    QString copyDefaultDestination() const;
    bool transferToArchivePanel(const QString &destination, bool move,
                                const CRecordVector<UInt32> &indices);
    bool transferBetweenArchivePanels(const QString &destination, bool move,
                                      const CRecordVector<UInt32> &indices);
    void rememberTransferDestination(const QString &path);
    void refreshTransferDestination(unsigned index);
    void createFileTable(QWidget *central);
    void configureColumns(bool initialized = false);
    void saveColumns();
    void showColumnsMenu(const QPoint &pos);
    void redrawTimestampColumns();
    void redrawSourceProperties();
    QString sourceColumnText(UInt32 itemIndex, int column);
    Qt::Alignment sourceColumnAlignment(int column) const;
    void createPanelActions(QMenu *editMenu);
    void createIconList(QWidget *central);
    void connectFilesystemScanner();
    void connectBackend();
    void backendFinished(bool ok, const QString &message, HRESULT result,
                         bool messageDisplayed = false);
    bool extractWithSource(const QString &destination, bool onlySelected, const QString &password,
                           const ExtractionOptions &options, const QStringList &names,
                           const CRecordVector<UInt32> *explicitIndices = nullptr);
    bool startPendingArchiveMove(bool ok);
    bool finishArchiveState(bool ok);
    void closeProgressDialog(bool finished = false);
    void beginWorkerProgress(const QString &label, const QString &detail,
                             std::function<void()> cancel);

    enum class State { Filesystem, Namespace, Loading, Viewing, Operating };
    enum class SortMode { Name, Type, Date, Size, Unsorted };
    void setViewMode(int mode);

  public:
    ~MainWindow() override;

  private:
    void startVolume(VolumeOperation::Mode mode);
    void showItemContextMenu(const QPoint &globalPos);
    void setSortMode(SortMode mode);
    void applySortMode(bool enableSorting = false);
    static QIcon toolbarIcon(int kind);
    static QString toolbarSettingsPath();
    void recordVisitedPath(const QString &path);
    void showFoldersHistory();
    void refreshBookmarks();
    void saveBookmark(int index);
    void openBookmark(int index);
    void updateToolbar();
    void applyFlatView(bool enabled);
    void showSettings();
    void applyFileListSettings(const CFmSettings &settings);
    void updateFilesystemIcons();
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool handlePanelKey(QObject *watched, QKeyEvent *key);
    bool handleSelectionKey(QKeyEvent *key);
    void handleSelectionClick(int row);
    void showDirectory(const QString &path, bool remember, bool sourceAlreadyBound = false);
    HRESULT renderRootFolder();
    void appendFilesystemEntry(const QFileInfo &file, quint32 sourceIndex);
    void appendParentEntry();
    void updateParentEntry();
    QStringList selectedPaths() const;
    void selectPaths(const QStringList &paths);
    void completeListSelection();
    void refreshCurrent(const CSelectedState *preserved = nullptr);
    HRESULT reloadPanelPresentation(unsigned index, const CSelectedState &selection);
    void startFilesystemListing(bool sourceAlreadyBound);
    void restoreListSelection(const CSelectedState &selection);
    struct FilesystemSignature;
    void pollAutoRefresh();
    void restoreAutoRefreshViewport();
    FilesystemSignature filesystemSignature(const QString &path) const;
    void createFolder();
    void createFile();
    void SelectAll(bool select);
    void InvertSelection();
    void SelectSpec(bool select);
    void SelectByType(bool select);
    HRESULT navigateTo(const QString &path, bool sourceAddress = false);
    void activateRow(int row);
    void goBack();
    void goUp();
    void goParent();
    enum class RootLocation { Namespace, Drives };
    void goRoot(bool remember = true, RootLocation location = RootLocation::Namespace);
    QString archiveDisplayPath() const;
    HRESULT renderBoundFolder();
    bool isNestedArchive() const;
    void showArchiveFolder(const QString &folder);
    HRESULT renderArchiveFolder();
    void appendArchiveEntry(CAgentFolder &folder, unsigned index, const QString &prefix, bool flat);
    void setState(State state);
    void refreshActions();
    void refreshPanelStatus();
    void showSelectedProperties();
    void calculateHash(const QString &method);
    void begin(const QString &label, const QString &detail);
    void startProgressDialog(const QString &label, const QString &detail);
    void openNestedArchive(const QString &entry);
    void extract(bool onlySelected);
    void createDialog();
    QStringList selectedFilesystemPaths();
    void transferFiles(const QStringList &sources, const QString &destination, bool move,
                       bool directPath = false, bool refreshSource = true);
    void executeCopyDestination(bool move, bool copyToSame, unsigned sourceIndex,
                                const QString &destination, const CRecordVector<UInt32> &indices,
                                bool useDestPanel, bool useTemp);
    void deleteSelected(bool toRecycleBin);
    void renameSelected();
    void finishFilesystemRename(quint32 sourceIndex, const QString &originalPath,
                                const QString &name);
    QStringList selectedArchivePaths();
    QStringList archivePathsForIndices(const CRecordVector<UInt32> &indices);
    QStringList selectedArchiveItemsPaths();
    CRecordVector<UInt32> sourceOperatedIndices();

    ArchiveBackend *backend_ = nullptr;
    std::unique_ptr<IQtTrashService> nativeTrashService_;
    IQtTrashService *trashService_ = nullptr;
    CMyComPtr<IFolderManager> folderManager_;
    std::unique_ptr<class QtAssociatedFileServices> associatedFileServices_;
    std::unique_ptr<class QtAppPresentation> appPresentation_;
    std::unique_ptr<class QtFileManagerDrag> drag_;
    std::unique_ptr<class QtFileManagerCommands> commands_;
    std::unique_ptr<class QtProgramFileMenu> programFileMenu_;
    std::function<bool(const QUrl &)> fileOpener_;
    QString archive_;
    QString displayArchive_;
    QString archivePassword_;
    QString archiveFolder_;
    CApp app_;
    QAction *saveBookmarks_[10]{};
    QAction *openBookmarks_[10]{};
    State state_ = State::Filesystem;
    SortMode sortMode_ = SortMode::Name;
    int sortColumn_ = 0;
    bool configuringColumns_ = false;
    Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
    QAction *sortActions_[5]{};
    QAction *timeActions_[5]{};
    QAction *utcTimeAction_ = nullptr;
    QAction *autoRefreshAction_ = nullptr;
    QAction *viewActions_[4]{};
    QAction *flatView_ = nullptr;
    QAction *twoPanelsAction_ = nullptr;
    bool flatDisk_ = false;
    bool flatArchive_ = false;
    bool scanningDisk_ = false;
    bool scanningArchive_ = false;
    QtFolderProjection *listProjection_ = nullptr;
    struct FilesystemSignature {
        QDateTime modified;
        QDateTime metadataChanged;
        qint64 size = 0;
        bool exists = false;
        bool directory = false;
        bool operator==(const FilesystemSignature &other) const {
            return modified == other.modified && metadataChanged == other.metadataChanged &&
                   size == other.size && exists == other.exists && directory == other.directory;
        }
    };
    struct PanelUiState {
        QWidget *container = nullptr;
        QLineEdit *path = nullptr;
        QComboBox *pathCombo = nullptr;
        QTableWidget *table = nullptr;
        QListView *iconList = nullptr;
        QStackedWidget *viewStack = nullptr;
        std::array<QLabel *, 4> statusLabels{};
        QtFileListHover *tableHover = nullptr;
        QtFileListHover *iconHover = nullptr;
        QString directory;
        QString returnDirectory;
        QString archive;
        QString displayArchive;
        QString archivePassword;
        QString archiveFolder;
        QStringList history;
        QList<ArchiveEntry> entries;
        ArchiveInfo info;
        State state = State::Filesystem;
        SortMode sortMode = SortMode::Name;
        int sortColumn = 0;
        Qt::SortOrder sortOrder = Qt::AscendingOrder;
        int viewMode = 3;
        bool flatDisk = false;
        bool flatArchive = false;
        bool scanningDisk = false;
        bool scanningArchive = false;
        FilesystemScanner *filesystemScanner = nullptr;
        QtFolderProjection *listProjection = nullptr;
        QHash<QString, FilesystemSignature> autoRefreshSnapshot;
        QString autoRefreshTopPath;
        int autoRefreshScrollValue = -1;
        std::optional<CSelectedState> pendingSelection;
        std::optional<QStringList> pendingSelectedPaths;
    };
    std::array<PanelUiState, 2> panelUi_;
    QSplitter *panelSplitter_ = nullptr;
    unsigned projectedPanel_ = 0;
    unsigned panelProjectionDepth_ = 0;
    int splitterPos_ = 0;
    QHash<QString, FilesystemSignature> autoRefreshSnapshot_;
    QString autoRefreshTopPath_;
    int autoRefreshScrollValue_ = -1;
    QTimer *autoRefreshTimer_ = nullptr;
    FilesystemScanner *filesystemScanner_ = nullptr;
    std::optional<CSelectedState> scanSelectedState_;
    std::optional<QStringList> scanSelectedPaths_;
    int viewMode_ = 3;
    State previousState_ = State::Filesystem;
    bool creating_ = false;
    bool updatingArchive_ = false;
    enum class ArchiveMoveStage { None, Extracting, Removing };
    ArchiveMoveStage archiveMoveStage_ = ArchiveMoveStage::None;
    QStringList archiveMovePaths_;
    std::optional<unsigned> transferDestinationPanel_;
    QList<ArchiveEntry> entries_;
    ArchiveInfo info_;
    QLineEdit *path_ = nullptr;
    QComboBox *pathCombo_ = nullptr;
    QString directory_;
    QString returnDirectory_;
    QStringList history_;
    QTableWidget *table_ = nullptr;
    QListView *iconList_ = nullptr;
    Qt::KeyboardModifiers selectionClickModifiers_ = Qt::NoModifier;
    bool selectionClickLeft_ = false;
    bool selectionShiftSequence_ = false;
    QStackedWidget *viewStack_ = nullptr;
    QPointer<CProgressDialog> progress_;
    std::unique_ptr<IQtProgressTransport> progressTransport_;
    bool sourceArchiveProgress_ = false;
    QString operationName_;
    QString currentDetail_;
    QAction *toolbarAdd_ = nullptr;
    QAction *extract_ = nullptr;
    QAction *test_ = nullptr;
    QAction *infoAction_ = nullptr;
    QAction *back_ = nullptr;
    QAction *up_ = nullptr;
    QAction *root_ = nullptr;
    QAction *openItem_ = nullptr;
    QAction *openInside_ = nullptr;
    QAction *openInsideOne_ = nullptr;
    QAction *openInsideParser_ = nullptr;
    QAction *openOutside_ = nullptr;
    QAction *exitAction_ = nullptr;
    QAction *parent_ = nullptr;
    QAction *selectAll_ = nullptr;
    QAction *copy_ = nullptr;
    QAction *move_ = nullptr;
    QAction *remove_ = nullptr;
    QAction *rename_ = nullptr;
    QTableWidgetItem *renameEditingItem_ = nullptr;
    QString renameOriginalPath_;
    quint32 renameSourceIndex_ = 0;
    bool renameActivationPending_ = false;
    QAction *split_ = nullptr;
    QAction *combine_ = nullptr;
    VolumeOperation *volumeOperation_ = nullptr;
    FileTransferOperation *fileTransfer_ = nullptr;
    QPointer<QDialog> overwriteDialog_;
    QAction *viewFile_ = nullptr;
    QAction *editFile_ = nullptr;
    QAction *diffFiles_ = nullptr;
    QAction *properties_ = nullptr;
    QAction *hashActions_[11]{};
    QAction *createFolder_ = nullptr;
    QAction *createFile_ = nullptr;
    QAction *refresh_ = nullptr;
    QAction *settingsAction_ = nullptr;
    CFmSettings fileListSettings_;
    QtFileListHover *tableHover_ = nullptr;
    QtFileListHover *iconHover_ = nullptr;
    QAction *foldersHistory_ = nullptr;
    QAction *deselectAll_ = nullptr;
    QAction *invertSelection_ = nullptr;
    QAction *selectMask_ = nullptr;
    QAction *deselectMask_ = nullptr;
    QAction *selectType_ = nullptr;
    QAction *deselectType_ = nullptr;
    QAction *archiveToolbarOption_ = nullptr;
    QAction *standardToolbarOption_ = nullptr;
    QAction *largeButtonsOption_ = nullptr;
    QAction *buttonTextOption_ = nullptr;
    QAction *toolbarGroupSeparator_ = nullptr;
};
