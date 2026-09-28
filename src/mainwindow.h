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
#include <QProgressDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
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
#include <functional>
#include <memory>
#include <vector>

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(ArchiveBackend *backend = nullptr,
                        std::function<bool(const QUrl &)> fileOpener = {});

    void openArchive(const QString &path, bool nested = false, const QString &label = {});

    void extractTo(const QString &destination, bool onlySelected = false,
                   const QString &password = {}, const ExtractionOptions &options = {});

    void testArchive();

  private:
    QMenu *createMenusAndToolbar();
    void createFileActions(QMenu *menu, QToolBar *toolbar);
    void createFileExtras(QMenu *menu, QToolBar *toolbar);
    void createViewModes(QMenu *viewMenu);
    void createFavoritesMenu(QMenu *favoritesMenu);
    void createViewOptions(QMenu *viewMenu);
    void createToolbarOptions(QMenu *viewMenu);
    void activateSingleClickedRow(int row);
    void createFilePanel(QMenu *editMenu);
    void createFileTable(QWidget *central);
    void createPanelActions(QMenu *editMenu);
    void createIconList(QWidget *central);
    void connectFilesystemScanner();
    void connectBackend();
    void backendFinished(bool ok, const QString &message);
    bool startPendingHash(bool ok);
    bool startPendingArchiveMove(bool ok);
    bool finishArchiveState(bool ok);
    void finishViewExtraction(bool ok);
    void closeProgressDialog();

    enum class State { Filesystem, Loading, Viewing, Operating };
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
    bool archiveFolderExists(const QString &folder) const;
    void updateToolbar();
    void showSettings();
    void updateFilesystemIcons();
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool handlePanelKey(QObject *watched, QKeyEvent *key);
    bool handleSelectionKey(QKeyEvent *key);
    void showDirectory(const QString &path, bool remember);
    void appendFilesystemEntry(const QFileInfo &file);
    QStringList selectedPaths() const;
    void selectPaths(const QStringList &paths);
    void refreshCurrent();
    void createFolder();
    void createFile();
    void SelectAll(bool select);
    void InvertSelection();
    void SelectSpec(bool select);
    void SelectByType(bool select);
    void navigateTo(const QString &path);
    void activateRow(int row);
    void openSelectedInside();
    void openSelectedOutside();
    void goBack();
    void goUp();
    void goParent();
    QString archiveDisplayPath() const;
    void restoreParentArchive();
    void showArchiveFolder(const QString &folder);
    void setState(State state);
    void refreshActions();
    void appendJournal(const QString &line);
    void showJournal();
    void showInfo();
    void showSelectedProperties();
    void calculateHash(const QString &method);
    void begin(const QString &label, const QString &detail, bool preserveJournal = false);
    void openDialog();
    void openFileViewer(const QString &path, bool editable);
    bool openLocalFile(const QString &path);
    void openNestedArchive(const QString &entry);
    bool finishNestedExtraction(bool ok);
    void openArchiveFile(const QString &entry);
    void viewSelectedFile(bool editable);
    void extract(bool onlySelected);
    void createDialog();
    void createArchive(const QString &target, const QStringList &sources,
                       const QString &password = {}, bool encryptHeaders = false,
                       const CompressionOptions &options = {});
    void addDialog();
    QStringList selectedFilesystemPaths() const;
    void transferFiles(const QStringList &sources, const QString &destination, bool move);
    void copySelected();
    void moveSelected();
    void transferSelected(bool move);
    QString selectedCopyInfo() const;
    void deleteSelected();
    void startFileDelete(const QStringList &paths);
    void renameSelected();
    void addSources(const QStringList &sources);
    QStringList selectedArchivePaths() const;

    ArchiveBackend *backend_ = nullptr;
    struct ArchiveParent {
        QString path;
        QString display;
        QString password;
        QString folder;
        QString entry;
        QList<ArchiveEntry> entries;
        ArchiveInfo info;
        std::unique_ptr<QTemporaryDir> temporary;
    };
    std::function<bool(const QUrl &)> fileOpener_;
    QString archive_;
    QString displayArchive_;
    std::vector<ArchiveParent> archiveParents_;
    std::unique_ptr<QTemporaryDir> nestedTemporary_;
    QString pendingNestedEntry_;
    bool pendingNested_ = false;
    bool nestedListing_ = false;
    QString archivePassword_;
    QString archiveFolder_;
    QString pendingArchiveFolder_;
    CApp app_;
    QAction *saveBookmarks_[10]{};
    QAction *openBookmarks_[10]{};
    State state_ = State::Filesystem;
    SortMode sortMode_ = SortMode::Name;
    int sortColumn_ = 0;
    Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
    QAction *sortActions_[5]{};
    QAction *viewActions_[4]{};
    QAction *flatView_ = nullptr;
    bool flatDisk_ = false;
    bool flatArchive_ = false;
    bool scanningDisk_ = false;
    FilesystemScanner *filesystemScanner_ = nullptr;
    QStringList scanSelection_;
    std::optional<CSelectedState> scanSelectedState_;
    int viewMode_ = 3;
    State previousState_ = State::Filesystem;
    bool creating_ = false;
    bool updatingArchive_ = false;
    enum class ArchiveMoveStage { None, Extracting, Removing };
    ArchiveMoveStage archiveMoveStage_ = ArchiveMoveStage::None;
    QStringList archiveMovePaths_;
    QList<ArchiveEntry> entries_;
    CProxyArc proxyArchive_;
    ArchiveInfo info_;
    QLineEdit *path_ = nullptr;
    QComboBox *pathCombo_ = nullptr;
    QString directory_;
    QString returnDirectory_;
    QStringList history_;
    bool refreshing_ = false;
    QString refreshFolder_;
    QStringList refreshSelection_;
    QTableWidget *table_ = nullptr;
    QListView *iconList_ = nullptr;
    QStackedWidget *viewStack_ = nullptr;
    QProgressDialog *progress_ = nullptr;
    QDialog *journalDialog_ = nullptr;
    QPointer<QDialog> infoDialog_;
    QPlainTextEdit *journalView_ = nullptr;
    QAction *journalAction_ = nullptr;
    QString journalText_;
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
    QAction *openItem_ = nullptr;
    QAction *openInside_ = nullptr;
    QAction *openOutside_ = nullptr;
    QAction *parent_ = nullptr;
    QAction *selectAll_ = nullptr;
    QAction *copy_ = nullptr;
    QAction *move_ = nullptr;
    QAction *remove_ = nullptr;
    QAction *rename_ = nullptr;
    QAction *split_ = nullptr;
    QAction *combine_ = nullptr;
    VolumeOperation *volumeOperation_ = nullptr;
    QProgressDialog *volumeProgress_ = nullptr;
    FileTransferOperation *fileTransfer_ = nullptr;
    FileDeleteOperation *fileDelete_ = nullptr;
    QProgressDialog *fileTransferProgress_ = nullptr;
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
