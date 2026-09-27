#pragma once

#include "archivebackend.h"
#include "combodialog.h"
#include "copydialog.h"
#include "createarchivedialog.h"
#include "extractdialog.h"
#include "filesystemscanner.h"
#include "fileviewerdialog.h"
#include "propertiesdialog.h"
#include "settingsdialog.h"
#include "volumedialog.h"

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

class FileTable : public QTableWidget {
  public:
    using QTableWidget::QTableWidget;
    bool filesystem = true;

  protected:
    QMimeData *mimeData(const QList<QTableWidgetItem *> &items) const override {
        auto *data = new QMimeData;
        if (!filesystem)
            return data;
        QList<QUrl> urls;
        for (QTableWidgetItem *item : items) {
            if (item->column() == 1)
                urls << QUrl::fromLocalFile(item->text());
        }
        data->setUrls(urls);
        return data;
    }
};

class ArchiveTableItem : public QTableWidgetItem {
  public:
    using QTableWidgetItem::QTableWidgetItem;
    static int compareNames(const QString &left, const QString &right) {
        const auto digit = [](QChar value) {
            return value >= QLatin1Char('0') && value <= QLatin1Char('9');
        };
        int leftIndex = 0;
        int rightIndex = 0;
        while (leftIndex < left.size() && rightIndex < right.size()) {
            if (digit(left[leftIndex]) && digit(right[rightIndex])) {
                while (leftIndex < left.size() && left[leftIndex] == QLatin1Char('0'))
                    ++leftIndex;
                while (rightIndex < right.size() && right[rightIndex] == QLatin1Char('0'))
                    ++rightIndex;
                int leftEnd = leftIndex;
                int rightEnd = rightIndex;
                while (leftEnd < left.size() && digit(left[leftEnd]))
                    ++leftEnd;
                while (rightEnd < right.size() && digit(right[rightEnd]))
                    ++rightEnd;
                const int leftLength = leftEnd - leftIndex;
                const int rightLength = rightEnd - rightIndex;
                if (leftLength != rightLength)
                    return leftLength < rightLength ? -1 : 1;
                const int numeric = QStringView(left)
                                        .mid(leftIndex, leftLength)
                                        .compare(QStringView(right).mid(rightIndex, rightLength));
                if (numeric != 0)
                    return numeric;
                leftIndex = leftEnd;
                rightIndex = rightEnd;
                continue;
            }
            const QChar leftChar = left[leftIndex++].toUpper();
            const QChar rightChar = right[rightIndex++].toUpper();
            if (leftChar != rightChar)
                return leftChar < rightChar ? -1 : 1;
        }
        if (leftIndex == left.size() && rightIndex == right.size())
            return 0;
        return leftIndex == left.size() ? -1 : 1;
    }
    bool operator<(const QTableWidgetItem &other) const override {
        if (data(Qt::UserRole + 1).toBool() != other.data(Qt::UserRole + 1).toBool())
            return data(Qt::UserRole + 1).toBool();
        if (data(Qt::UserRole + 2).isValid() && other.data(Qt::UserRole + 2).isValid()) {
            const qint64 left = data(Qt::UserRole + 2).toLongLong();
            const qint64 right = other.data(Qt::UserRole + 2).toLongLong();
            if (left != right)
                return left < right;
        }
        const int value = compareNames(text(), other.text());
        if (value != 0)
            return value < 0;
        return compareNames(data(Qt::UserRole + 3).toString(),
                            other.data(Qt::UserRole + 3).toString()) < 0;
    }
};

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(ArchiveBackend *backend = nullptr,
                        std::function<bool(const QUrl &)> fileOpener = {});

    void openArchive(const QString &path, bool nested = false, const QString &label = {}) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix != "7z" && suffix != "zip") {
            QMessageBox::warning(this, "Sevenzip Desktop",
                                 "Поддерживаются только архивы .7z и .zip.");
            return;
        }
        filesystemScanner_->cancel();
        if (!nested)
            archiveParents_.clear();
        scanningDisk_ = false;
        scanSelection_.clear();
        archive_ = QFileInfo(path).absoluteFilePath();
        displayArchive_ = nested ? label : archive_;
        archivePassword_.clear();
        if (infoDialog_)
            infoDialog_->close();
        if (state_ == State::Filesystem)
            returnDirectory_ = directory_;
        else if (returnDirectory_.isEmpty())
            returnDirectory_ = QFileInfo(archive_).absolutePath();
        entries_.clear();
        info_ = ArchiveInfo{};
        archiveFolder_.clear();
        refreshing_ = false;
        refreshFolder_.clear();
        refreshSelection_.clear();
        table_->setRowCount(0);
        path_->setText(archive_);
        setState(State::Loading);
        begin("Открытие архива", archive_);
        nestedListing_ = nested;
        backend_->list(archive_);
    }

    void extractTo(const QString &destination, bool onlySelected = false,
                   const QString &password = {}, const ExtractionOptions &options = {}) {
        if (state_ != State::Viewing || entries_.isEmpty() || destination.isEmpty())
            return;
        QStringList names;
        if (onlySelected) {
            names = selectedArchivePaths();
            if (names.isEmpty())
                return;
        }
        previousState_ = state_;
        setState(State::Operating);
        begin("Извлечение", destination);
        backend_->extract(archive_, destination, names,
                          password.isEmpty() ? archivePassword_ : password, options);
    }

    void testArchive() {
        if (state_ != State::Viewing)
            return;
        previousState_ = state_;
        setState(State::Operating);
        begin("Проверка архива", archive_);
        backend_->test(archive_, archivePassword_);
    }

  private:
    enum class State { Filesystem, Loading, Viewing, Operating };
    enum class SortMode { Name, Type, Date, Size, Unsorted };
    void setViewMode(int mode) {
        if (mode < 0 || mode > 3 || !viewStack_)
            return;
        viewMode_ = mode;
        viewActions_[mode]->setChecked(true);
        if (mode != 3) {
            iconList_->setViewMode(mode == 2 ? QListView::ListMode : QListView::IconMode);
            iconList_->setIconSize(mode == 0 ? QSize(48, 48) : QSize(16, 16));
            iconList_->setGridSize(mode == 0   ? QSize(140, 82)
                                   : mode == 1 ? QSize(180, 26)
                                               : QSize());
            iconList_->setFlow(mode == 2 ? QListView::TopToBottom : QListView::LeftToRight);
            iconList_->setWrapping(true);
            iconList_->setMovement(QListView::Static);
        }
        viewStack_->setCurrentIndex(mode == 3 ? 0 : 1);
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        settings.setValue("view/mode", mode);
    }

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
    void showFoldersHistory();
    struct Bookmark {
        QString path;
        QString folder;
    };
    void refreshBookmarks();
    void saveBookmark(int index);
    void openBookmark(int index);
    bool archiveFolderExists(const QString &folder) const;
    void updateToolbar();
    void showSettings();
    void updateFilesystemIcons();
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showDirectory(const QString &path, bool remember);
    void appendFilesystemEntry(const QFileInfo &file);
    QStringList selectedPaths() const;
    void selectPaths(const QStringList &paths);
    void refreshCurrent();
    void createFolder();
    void createFile();
    void selectByMask(bool select);
    void selectByType(bool select);
    void navigateTo(const QString &path);
    void activateRow(int row);
    void openSelectedInside();
    void openSelectedOutside();
    void goBack() {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (state_ == State::Viewing) {
            if (archiveFolder_.isEmpty()) {
                if (archiveParents_.empty())
                    showDirectory(returnDirectory_, false);
                else
                    restoreParentArchive();
            } else
                goUp();
        } else if (!history_.isEmpty()) {
            showDirectory(history_.takeLast(), false);
        }
    }
    void goUp() {
        if (state_ != State::Viewing || archiveFolder_.isEmpty())
            return;
        const int separator = archiveFolder_.lastIndexOf('/');
        showArchiveFolder(separator < 0 ? QString() : archiveFolder_.left(separator));
    }
    void goParent() {
        if (state_ == State::Viewing) {
            goBack();
        } else if (state_ == State::Filesystem) {
            QDir parent(directory_);
            if (parent.cdUp())
                showDirectory(parent.absolutePath(), true);
        }
    }
    QString archiveDisplayPath() const {
        return displayArchive_ +
               (archiveFolder_.isEmpty() ? QString() : QStringLiteral(" / ") + archiveFolder_);
    }
    void restoreParentArchive();
    void showArchiveFolder(const QString &folder);
    void setState(State state);
    void refreshActions();
    void appendJournal(const QString &line);
    void showJournal();
    void showInfo();
    void showSelectedProperties();
    void begin(const QString &label, const QString &detail);
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
    static bool copyTree(const QString &source, const QString &target);
    static void removeTree(const QString &path);
    void transferFiles(const QStringList &sources, const QString &destination, bool move);
    void copySelected();
    void moveSelected();
    void transferSelected(bool move);
    void deleteSelected();
    void renameSelected();
    void addSources(const QStringList &sources) {
        if (state_ != State::Viewing || info_.encrypted || sources.isEmpty())
            return;
        previousState_ = state_;
        updatingArchive_ = true;
        setState(State::Operating);
        begin("Добавление в архив", archive_);
        backend_->add(archive_, sources);
    }
    QStringList selectedArchivePaths() const {
        QStringList paths;
        if (state_ != State::Viewing)
            return paths;
        for (const QModelIndex &index : table_->selectionModel()->selectedRows()) {
            const auto *item = table_->item(index.row(), 0);
            const QString path = item->data(Qt::UserRole).toString();
            if (!item->data(Qt::UserRole + 1).toBool()) {
                paths << path;
                continue;
            }
            for (const ArchiveEntry &entry : entries_) {
                QString normalized = entry.path;
                if (normalized.endsWith('/'))
                    normalized.chop(1);
                if (normalized == path || normalized.startsWith(path + '/'))
                    paths << entry.path;
            }
        }
        paths.removeDuplicates();
        return paths;
    }

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
    QString pendingBookmarkFolder_;
    Bookmark bookmarks_[10];
    QAction *saveBookmarks_[10]{};
    QAction *openBookmarks_[10]{};
    State state_ = State::Filesystem;
    SortMode sortMode_ = SortMode::Name;
    QAction *sortActions_[5]{};
    QAction *viewActions_[4]{};
    QAction *flatView_ = nullptr;
    bool flatDisk_ = false;
    bool flatArchive_ = false;
    bool scanningDisk_ = false;
    FilesystemScanner *filesystemScanner_ = nullptr;
    QStringList scanSelection_;
    int viewMode_ = 3;
    State previousState_ = State::Filesystem;
    bool creating_ = false;
    bool updatingArchive_ = false;
    QList<ArchiveEntry> entries_;
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
    QAction *viewFile_ = nullptr;
    QAction *editFile_ = nullptr;
    QAction *properties_ = nullptr;
    QAction *createFolder_ = nullptr;
    QAction *createFile_ = nullptr;
    QAction *refresh_ = nullptr;
    QAction *settingsAction_ = nullptr;
    FileListSettings fileListSettings_;
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
