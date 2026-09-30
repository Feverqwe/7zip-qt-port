#pragma once
#include <optional>

#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/App.h"
#include "archivebackend.h"
#include "archiveformats.h"
#include "combodialog.h"
#include "copydialog.h"
#include "createarchivedialog.h"
#include "extractdialog.h"
#include "filedeleteoperation.h"
#include "filesystemscanner.h"
#include "filetable.h"
#include "filetransferoperation.h"
#include "fileviewerdialog.h"
#include "propertiesdialog.h"
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
#include <QTemporaryDir>
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
class CMessagesDialog;
class IQtProgressTransport;

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(ArchiveBackend *backend = nullptr,
                        std::function<bool(const QUrl &)> fileOpener = {});

    void openArchive(const QString &path);

    void extractTo(const QString &destination, bool onlySelected = false,
                   const QString &password = {}, const ExtractionOptions &options = {});

    void testArchive();

  private:
    friend class QtAppPresentation;
    friend class QtFileManagerWindowServices;
    friend class QtFileManagerCommands;
    void closeEvent(QCloseEvent *event) override;
    void quiesceOperations();
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
    unsigned panelIndexFor(QObject *object) const;
    void togglePanels();
    QString copyDefaultDestination() const;
    void createFileTable(QWidget *central);
    void configureColumns();
    void saveColumns();
    void showColumnsMenu(const QPoint &pos);
    void redrawTimestampColumns();
    QString sourceColumnText(UInt32 itemIndex, int column);
    Qt::Alignment sourceColumnAlignment(int column) const;
    void createPanelActions(QMenu *editMenu);
    void createIconList(QWidget *central);
    void connectFilesystemScanner();
    void connectBackend();
    void backendFinished(bool ok, const QString &message, bool messageDisplayed = false);
    bool startPendingHash(bool ok);
    bool startPendingArchiveMove(bool ok);
    bool finishArchiveState(bool ok);
    void finishViewExtraction(bool ok);
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
    void applySortMode();
    static QIcon toolbarIcon(int kind);
    static QString toolbarSettingsPath();
    void recordVisitedPath(const QString &path);
    void refreshPathHistory();
    void showFoldersHistory();
    void refreshBookmarks();
    void saveBookmark(int index);
    void openBookmark(int index);
    void updateToolbar();
    void applyFlatView(bool enabled);
    void showSettings();
    void updateFilesystemIcons();
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool handlePanelKey(QObject *watched, QKeyEvent *key);
    bool handleSelectionKey(QKeyEvent *key);
    void handleSelectionClick(int row);
    void showDirectory(const QString &path, bool remember, bool sourceAlreadyBound = false);
    void renderRootFolder();
    void appendFilesystemEntry(const QFileInfo &file, quint32 sourceIndex);
    void appendParentEntry();
    void updateParentEntry();
    QStringList selectedPaths() const;
    void selectPaths(const QStringList &paths);
    void refreshCurrent(const CSelectedState *preserved = nullptr);
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
    void navigateTo(const QString &path);
    void activateRow(int row);
    void goBack();
    void goUp();
    void goParent();
    void goRoot(bool remember = true);
    QString archiveDisplayPath() const;
    void renderBoundFolder();
    bool isNestedArchive() const;
    void showArchiveFolder(const QString &folder);
    void renderArchiveFolder();
    void appendArchiveEntry(CAgentFolder &folder, unsigned index, const QString &prefix, bool flat);
    void setState(State state);
    void refreshActions();
    void refreshPanelStatus();
    void appendJournal(const QString &line);
    void showJournal();
    void showInfo();
    void showSelectedProperties();
    void calculateHash(const QString &method);
    void begin(const QString &label, const QString &detail, bool preserveJournal = false);
    void startProgressDialog(const QString &label, const QString &detail);
    void openDialog();
    void openFileViewer(const QString &path, bool editable);
    bool openLocalFile(const QString &path);
    void openNestedArchive(const QString &entry);
    void openArchiveFile(const QString &entry);
    void viewSelectedFile(bool editable);
    void extract(bool onlySelected);
    void createDialog();
    void createArchive(const QString &target, const QStringList &sources,
                       const QString &password = {}, bool encryptHeaders = false,
                       const CompressionOptions &options = {});
    void addDialog();
    QStringList selectedFilesystemPaths();
    void transferFiles(const QStringList &sources, const QString &destination, bool move);
    void copySelected();
    void moveSelected();
    void transferSelected(bool move);
    QString selectedCopyInfo();
    void deleteSelected();
    void startFileDelete(const QStringList &paths);
    void renameSelected();
    void addSources(const QStringList &sources);
    QStringList selectedArchivePaths();
    QStringList selectedArchiveItemsPaths();
    CRecordVector<UInt32> sourceOperatedIndices();

    ArchiveBackend *backend_ = nullptr;
    CMyComPtr<IFolderManager> folderManager_;
    std::unique_ptr<class QtAssociatedFileServices> associatedFileServices_;
    std::unique_ptr<class QtAppPresentation> appPresentation_;
    std::unique_ptr<class QtFileManagerCommands> commands_;
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
        QHash<QString, FilesystemSignature> autoRefreshSnapshot;
        QString autoRefreshTopPath;
        int autoRefreshScrollValue = -1;
        bool needsRescan = false;
    };
    std::array<PanelUiState, 2> panelUi_;
    QSplitter *panelSplitter_ = nullptr;
    unsigned projectedPanel_ = 0;
    int splitterPos_ = 0;
    QHash<QString, FilesystemSignature> autoRefreshSnapshot_;
    QString autoRefreshTopPath_;
    int autoRefreshScrollValue_ = -1;
    QTimer *autoRefreshTimer_ = nullptr;
    FilesystemScanner *filesystemScanner_ = nullptr;
    std::optional<CSelectedState> scanSelectedState_;
    int viewMode_ = 3;
    State previousState_ = State::Filesystem;
    bool creating_ = false;
    bool updatingArchive_ = false;
    enum class ArchiveMoveStage { None, Extracting, Removing };
    ArchiveMoveStage archiveMoveStage_ = ArchiveMoveStage::None;
    QStringList archiveMovePaths_;
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
    QPointer<CMessagesDialog> journalDialog_;
    QPointer<QDialog> infoDialog_;
    QAction *journalAction_ = nullptr;
    UStringVector journalMessages_;
    QString operationName_;
    QString currentDetail_;
    QAction *open_ = nullptr;
    QAction *create_ = nullptr;
    QAction *add_ = nullptr;
    QAction *toolbarAdd_ = nullptr;
    QAction *extract_ = nullptr;
    QAction *test_ = nullptr;
    QAction *infoAction_ = nullptr;
    QAction *selected_ = nullptr;
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
    QAction *split_ = nullptr;
    QAction *combine_ = nullptr;
    VolumeOperation *volumeOperation_ = nullptr;
    FileTransferOperation *fileTransfer_ = nullptr;
    QPointer<QDialog> overwriteDialog_;
    quint64 fileTransferCompletedBytes_ = 0;
    FileDeleteOperation *fileDelete_ = nullptr;
    QAction *viewFile_ = nullptr;
    QAction *editFile_ = nullptr;
    QAction *properties_ = nullptr;
    QAction *hashActions_[11]{};
    bool pendingHashExtraction_ = false;
    bool hashing_ = false;
    QString hashMethod_;
    QStringList hashSelection_;
    std::unique_ptr<QTemporaryDir> hashTemporary_;
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
    std::unique_ptr<QTemporaryDir> viewTemporary_;
    std::vector<std::unique_ptr<QTemporaryDir>> openedTemporaries_;
    QString pendingViewPath_;
    bool pendingView_ = false;
    bool pendingExternalOpen_ = false;
    bool pendingViewEditable_ = false;
    QString pendingEditEntry_;
    bool replacingArchiveEntry_ = false;
};
