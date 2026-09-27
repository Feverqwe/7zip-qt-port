#pragma once

#include "archivebackend.h"
#include "combodialog.h"
#include "copydialog.h"
#include "createarchivedialog.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
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
#include <QSizePolicy>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringView>
#include <QStyle>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>

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
    explicit MainWindow(ArchiveBackend *backend = nullptr) {
        backend_ = backend ? backend : new ArchiveBackend(this);
        setWindowTitle("Sevenzip Desktop");
        resize(980, 620);
        auto *menu = menuBar()->addMenu("Файл");
        auto *toolbar = addToolBar("Архив");
        toolbar->setObjectName("archiveToolbar");
        toolbar->setMovable(false);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        toolbar->setIconSize(QSize(24, 24));
        back_ = new QAction("Назад", this);
        back_->setObjectName("backAction");
        connect(back_, &QAction::triggered, this, [this] { goBack(); });
        up_ = new QAction("Вверх", this);
        up_->setObjectName("upAction");
        connect(up_, &QAction::triggered, this, [this] { goUp(); });
        auto *open = menu->addAction("Открыть…", this, [this] { openDialog(); });
        open->setObjectName("openArchiveAction");
        open->setShortcut(QKeySequence::Open);
        create_ = menu->addAction("Создать архив…", this, [this] { createDialog(); });
        create_->setObjectName("createArchiveAction");
        add_ = menu->addAction("Добавить в открытый архив…", this, [this] { addDialog(); });
        add_->setObjectName("addArchiveAction");
        toolbarAdd_ = toolbar->addAction("Добавить", this, [this] {
            if (state_ == State::Filesystem)
                createDialog();
            else if (state_ == State::Viewing)
                addDialog();
        });
        toolbarAdd_->setObjectName("toolbarAddAction");
        extract_ = toolbar->addAction("Извлечь", this, [this] { extract(false); });
        extract_->setObjectName("extractArchiveAction");
        selected_ = menu->addAction("Извлечь выбранное…", this, [this] { extract(true); });
        selected_->setObjectName("extractSelectedAction");
        journalAction_ =
            menu->addAction("Журнал последней операции…", this, [this] { showJournal(); });
        journalAction_->setObjectName("operationJournalAction");
        journalAction_->setEnabled(false);
        test_ = toolbar->addAction("Тест", this, [this] { testArchive(); });
        test_->setObjectName("testArchiveAction");
        copy_ = toolbar->addAction("Копировать", this, [this] { copySelected(); });
        copy_->setObjectName("copyAction");
        move_ = toolbar->addAction("Переместить", this, [this] { moveSelected(); });
        move_->setObjectName("moveAction");
        remove_ = toolbar->addAction("Удалить", this, [this] { deleteSelected(); });
        remove_->setObjectName("deleteAction");
        rename_ = menu->addAction("Переименовать…", this, [this] { renameSelected(); });
        rename_->setObjectName("renameAction");
        properties_ = menu->addAction("Свойства…", this, [this] { showSelectedProperties(); });
        properties_->setObjectName("selectedPropertiesAction");
        properties_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));
        createFolder_ = menu->addAction("Создать папку…", this, [this] { createFolder(); });
        createFolder_->setObjectName("createFolderAction");
        createFolder_->setShortcut(QKeySequence(Qt::Key_F7));
        createFile_ = menu->addAction("Создать файл…", this, [this] { createFile(); });
        createFile_->setObjectName("createFileAction");
        createFile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
        copy_->setShortcut(QKeySequence(Qt::Key_F5));
        move_->setShortcut(QKeySequence(Qt::Key_F6));
        remove_->setShortcut(QKeySequence::Delete);
        rename_->setShortcut(QKeySequence(Qt::Key_F2));
        infoAction_ = toolbar->addAction("Информация", this, [this] { showInfo(); });
        infoAction_->setObjectName("archiveInfoAction");
        toolbarAdd_->setIcon(toolbarIcon(0));
        extract_->setIcon(toolbarIcon(1));
        test_->setIcon(toolbarIcon(2));
        copy_->setIcon(toolbarIcon(3));
        move_->setIcon(toolbarIcon(4));
        remove_->setIcon(toolbarIcon(5));
        infoAction_->setIcon(toolbarIcon(6));
        toolbarGroupSeparator_ = toolbar->insertSeparator(copy_);
        toolbarGroupSeparator_->setObjectName("toolbarGroupSeparator");
        for (const QString &title :
             {"Пароль", "Изменить архив", "Интеграция с файловым менеджером"})
            menu->addAction(title)->setEnabled(false);
        open_ = open;
        auto *editMenu = menuBar()->addMenu("Правка");
        auto *viewMenu = menuBar()->addMenu("Вид");
        auto *favoritesMenu = menuBar()->addMenu("Избранное");
        auto *addFavoriteMenu = favoritesMenu->addMenu("Добавить папку в избранное как");
        QSettings favoritesSettings(toolbarSettingsPath(), QSettings::IniFormat);
        for (int index = 0; index < 10; ++index) {
            const QString key = QStringLiteral("favorites/%1/").arg(index);
            bookmarks_[index].path = favoritesSettings.value(key + "path").toString();
            bookmarks_[index].folder = favoritesSettings.value(key + "folder").toString();
            auto *save = addFavoriteMenu->addAction(QString::number(index), this,
                                                    [this, index] { saveBookmark(index); });
            save->setObjectName(QStringLiteral("saveBookmark%1Action").arg(index));
            auto *open = favoritesMenu->addAction(QString::number(index), this,
                                                  [this, index] { openBookmark(index); });
            open->setObjectName(QStringLiteral("openBookmark%1Action").arg(index));
            saveBookmarks_[index] = save;
            openBookmarks_[index] = open;
        }
        refreshBookmarks();
        auto *sortGroup = new QActionGroup(this);
        const QStringList sortLabels = {"По имени", "По типу", "По дате", "По размеру",
                                        "Без сортировки"};
        for (int index = 0; index < sortLabels.size(); ++index) {
            QAction *action = viewMenu->addAction(sortLabels[index]);
            action->setObjectName(QStringLiteral("sort%1Action").arg(index));
            action->setCheckable(true);
            action->setActionGroup(sortGroup);
            action->setShortcut(QKeySequence(Qt::CTRL | (Qt::Key_F3 + index)));
            sortActions_[index] = action;
            connect(action, &QAction::triggered, this,
                    [this, index] { setSortMode(static_cast<SortMode>(index)); });
        }
        sortActions_[0]->setChecked(true);
        viewMenu->addSeparator();
        auto *toolbarsMenu = viewMenu->addMenu("Панели инструментов");
        auto addToolbarOption = [this, toolbarsMenu](const QString &title, const char *name,
                                                     bool checked) {
            auto *option = toolbarsMenu->addAction(title);
            option->setObjectName(name);
            option->setCheckable(true);
            option->setChecked(checked);
            connect(option, &QAction::toggled, this, [this] { updateToolbar(); });
            return option;
        };
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        archiveToolbarOption_ = addToolbarOption("Архивная панель", "archiveToolbarOption",
                                                 settings.value("toolbar/archive", true).toBool());
        standardToolbarOption_ =
            addToolbarOption("Стандартная панель", "standardToolbarOption",
                             settings.value("toolbar/standard", true).toBool());
        largeButtonsOption_ = addToolbarOption("Крупные значки", "largeButtonsOption",
                                               settings.value("toolbar/large", false).toBool());
        buttonTextOption_ = addToolbarOption("Подписи кнопок", "buttonTextOption",
                                             settings.value("toolbar/text", true).toBool());
        updateToolbar();
        refresh_ = viewMenu->addAction("Обновить", this, [this] { refreshCurrent(); });
        refresh_->setObjectName("refreshAction");
        refresh_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
        foldersHistory_ =
            viewMenu->addAction("История папок…", this, [this] { showFoldersHistory(); });
        foldersHistory_->setObjectName("foldersHistoryAction");
        foldersHistory_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_F12));

        auto *central = new QWidget(this);
        auto *layout = new QVBoxLayout(central);
        layout->setContentsMargins(4, 4, 4, 4);
        auto *navigation = new QHBoxLayout;
        navigation->setContentsMargins(0, 0, 0, 0);
        for (QAction *action : {back_, up_}) {
            auto *button = new QToolButton(central);
            button->setDefaultAction(action);
            navigation->addWidget(button);
        }
        pathCombo_ = new QComboBox(central);
        pathCombo_->setObjectName("pathCombo");
        pathCombo_->setEditable(true);
        pathCombo_->setInsertPolicy(QComboBox::NoInsert);
        path_ = pathCombo_->lineEdit();
        path_->setObjectName("archivePath");
        path_->setPlaceholderText("Путь к папке или архиву");
        for (const QString &visited : settings.value("history/paths").toStringList()) {
            if (pathCombo_->count() >= 20)
                break;
            pathCombo_->addItem(visited);
        }
        navigation->addWidget(pathCombo_);
        layout->addLayout(navigation);
        table_ = new FileTable(0, 7, central);
        table_->setObjectName("archiveTable");
        table_->verticalHeader()->hide();
        table_->setHorizontalHeaderLabels(
            {"Имя", "Путь", "Размер", "Сжатый", "Изменён", "Тип", "Порядок"});
        table_->setColumnHidden(1, true);
        table_->setColumnHidden(5, true);
        table_->setColumnHidden(6, true);
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_->setColumnWidth(2, 100);
        table_->setColumnWidth(3, 100);
        table_->setColumnWidth(4, 150);
        table_->setShowGrid(false);
        table_->setWordWrap(false);
        table_->setTextElideMode(Qt::ElideMiddle);
        for (int column : {2, 3, 4})
            table_->setColumnHidden(
                column, !settings.value(QStringLiteral("columns/%1").arg(column), true).toBool());
        table_->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(table_->horizontalHeader(), &QWidget::customContextMenuRequested, this,
                [this](const QPoint &pos) {
                    QMenu menu(this);
                    for (int column : {2, 3, 4}) {
                        if (column == 3 && state_ == State::Filesystem)
                            continue;
                        QAction *option =
                            menu.addAction(table_->horizontalHeaderItem(column)->text());
                        option->setCheckable(true);
                        option->setChecked(!table_->isColumnHidden(column));
                        connect(option, &QAction::toggled, this, [this, column](bool visible) {
                            table_->setColumnHidden(column, !visible);
                            QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
                            settings.setValue(QStringLiteral("columns/%1").arg(column), visible);
                        });
                    }
                    menu.exec(table_->horizontalHeader()->mapToGlobal(pos));
                });
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_->setSortingEnabled(true);
        table_->setDragEnabled(true);
        table_->setAcceptDrops(true);
        table_->viewport()->setAcceptDrops(true);
        table_->sortItems(0, Qt::AscendingOrder);
        connect(table_->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this,
                [this](int column, Qt::SortOrder) {
                    const int mode = column == 5   ? 1
                                     : column == 4 ? 2
                                     : column == 2 ? 3
                                     : column == 6 ? 4
                                                   : 0;
                    sortMode_ = static_cast<SortMode>(mode);
                    sortActions_[mode]->setChecked(true);
                });
        table_->installEventFilter(this);
        table_->viewport()->installEventFilter(this);
        table_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(table_, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            const int row = table_->rowAt(pos.y());
            if (row >= 0 && !table_->selectionModel()->isRowSelected(row, QModelIndex()))
                table_->selectRow(row);
            QMenu menu(this);
            const bool hasSelection = !table_->selectionModel()->selectedRows().isEmpty();
            if (hasSelection) {
                for (QAction *action : {openItem_, rename_, copy_, move_, remove_})
                    menu.addAction(action);
                menu.addSeparator();
                menu.addAction(properties_);
                menu.addSeparator();
            }
            if (state_ == State::Filesystem) {
                menu.addAction(create_);
                menu.addAction(createFolder_);
                menu.addAction(createFile_);
            } else {
                menu.addAction(add_);
                if (hasSelection)
                    menu.addAction(selected_);
                menu.addAction(extract_);
                menu.addAction(test_);
                menu.addAction(infoAction_);
            }
            menu.addSeparator();
            menu.addAction(refresh_);
            menu.exec(table_->viewport()->mapToGlobal(pos));
        });
        openItem_ = new QAction(table_);
        openItem_->setObjectName("openItemAction");
        table_->addAction(openItem_);
        connect(openItem_, &QAction::triggered, this,
                [this] { activateRow(table_->currentRow()); });
        parent_ = new QAction(table_);
        parent_->setObjectName("parentFolderAction");
        table_->addAction(parent_);
        connect(parent_, &QAction::triggered, this, [this] { goParent(); });
        selectAll_ = new QAction(table_);
        selectAll_->setObjectName("selectAllAction");
        selectAll_->setShortcut(QKeySequence::SelectAll);
        selectAll_->setShortcutContext(Qt::WidgetShortcut);
        table_->addAction(selectAll_);
        connect(selectAll_, &QAction::triggered, table_, &QTableWidget::selectAll);
        selectAll_->setText("Выделить всё");
        editMenu->addAction(selectAll_);
        deselectAll_ =
            editMenu->addAction("Снять выделение", table_, [this] { table_->clearSelection(); });
        deselectAll_->setObjectName("deselectAllAction");
        invertSelection_ = editMenu->addAction("Инвертировать выделение", table_, [this] {
            for (int row = 0; row < table_->rowCount(); ++row)
                table_->selectionModel()->select(table_->model()->index(row, 0),
                                                 QItemSelectionModel::Toggle |
                                                     QItemSelectionModel::Rows);
        });
        invertSelection_->setObjectName("invertSelectionAction");
        selectMask_ =
            editMenu->addAction("Выделить по маске…", this, [this] { selectByMask(true); });
        selectMask_->setObjectName("selectMaskAction");
        deselectMask_ =
            editMenu->addAction("Снять выделение по маске…", this, [this] { selectByMask(false); });
        deselectMask_->setObjectName("deselectMaskAction");
        selectType_ = editMenu->addAction("Выделить по типу", this, [this] { selectByType(true); });
        selectType_->setObjectName("selectTypeAction");
        deselectType_ =
            editMenu->addAction("Снять выделение по типу", this, [this] { selectByType(false); });
        deselectType_->setObjectName("deselectTypeAction");
        layout->addWidget(table_);
        setCentralWidget(central);
        statusBar()->showMessage("Готово");
        connect(path_, &QLineEdit::returnPressed, this, [this] { navigateTo(path_->text()); });
        connect(pathCombo_, &QComboBox::activated, this,
                [this](int index) { navigateTo(pathCombo_->itemText(index)); });
        connect(table_, &QTableWidget::cellDoubleClicked, this,
                [this](int row, int) { activateRow(row); });
        showDirectory(QDir::currentPath(), false);
        refreshActions();
        connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                [this] { refreshActions(); });
        connect(table_, &QTableWidget::currentCellChanged, this, [this] { refreshActions(); });

        connect(backend_, &ArchiveBackend::entriesReady, this,
                [this](const QList<ArchiveEntry> &entries) {
                    if (state_ != State::Loading)
                        return;
                    entries_ = entries;
                    QString folder = refreshing_ ? refreshFolder_ : pendingBookmarkFolder_;
                    while (!folder.isEmpty()) {
                        bool present = false;
                        for (const ArchiveEntry &entry : entries_) {
                            present = entry.path == folder || entry.path == folder + '/' ||
                                      entry.path.startsWith(folder + '/');
                            if (present)
                                break;
                        }
                        if (present)
                            break;
                        const int separator = folder.lastIndexOf('/');
                        folder = separator < 0 ? QString() : folder.left(separator);
                    }
                    showArchiveFolder(folder);
                    pendingBookmarkFolder_.clear();
                    if (refreshing_)
                        selectPaths(refreshSelection_);
                    refreshing_ = false;
                    refreshFolder_.clear();
                    refreshSelection_.clear();
                    setState(State::Viewing);
                });
        connect(backend_, &ArchiveBackend::infoReady, this,
                [this](const ArchiveInfo &info) { info_ = info; });
        connect(backend_, &ArchiveBackend::passwordRequired, this, [this] {
            bool accepted = false;
            const QString password = QInputDialog::getText(
                this, "Пароль архива", "Введите пароль:", QLineEdit::Password, {}, &accepted);
            if (!accepted || password.isEmpty()) {
                backend_->cancel();
                return;
            }
            archivePassword_ = password;
            backend_->providePassword(password);
        });
        connect(backend_, &ArchiveBackend::outputReceived, this,
                [this](const QString &line) { appendJournal(line); });
        connect(backend_, &ArchiveBackend::progress, this,
                [this](int percent, const QString &detail) {
                    if (!progress_)
                        return;
                    if (percent >= 0)
                        progress_->setValue(percent);
                    if (!detail.isEmpty()) {
                        currentDetail_ = detail;
                        progress_->setLabelText(operationName_ + "\n" + currentDetail_);
                        statusBar()->showMessage(operationName_ + ": " + currentDetail_);
                    }
                });
        connect(backend_, &ArchiveBackend::finished, this, [this](bool ok, const QString &message) {
            if (!ok)
                archivePassword_.clear();
            appendJournal((ok ? QStringLiteral("Результат: ") : QStringLiteral("Ошибка: ")) +
                          message);
            if (progress_) {
                QProgressDialog *completed = progress_;
                progress_ = nullptr;
                completed->disconnect(backend_);
                completed->close();
                completed->deleteLater();
            }
            if (state_ == State::Loading && !ok)
                showDirectory(returnDirectory_, false);
            else if (state_ == State::Operating) {
                if (creating_)
                    showDirectory(directory_, false);
                else if (updatingArchive_ && ok) {
                    updatingArchive_ = false;
                    entries_.clear();
                    table_->setRowCount(0);
                    setState(State::Loading);
                    backend_->list(archive_);
                    return;
                } else
                    setState(previousState_ == State::Viewing ? State::Viewing : State::Filesystem);
                creating_ = false;
                updatingArchive_ = false;
            }
            statusBar()->showMessage(message);
            if (!ok && message != "Операция отменена.")
                QMessageBox::warning(this, "Sevenzip Desktop", message);
        });
    }

    void openArchive(const QString &path) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix != "7z" && suffix != "zip") {
            QMessageBox::warning(this, "Sevenzip Desktop",
                                 "Поддерживаются только архивы .7z и .zip.");
            return;
        }
        archive_ = QFileInfo(path).absoluteFilePath();
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
        backend_->list(archive_);
    }

    void extractTo(const QString &destination, bool onlySelected = false) {
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
        backend_->extract(archive_, destination, names, archivePassword_);
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
    void setSortMode(SortMode mode) {
        sortMode_ = mode;
        sortActions_[static_cast<int>(mode)]->setChecked(true);
        applySortMode();
    }
    void applySortMode() {
        if (!table_)
            return;
        const int columns[] = {0, 5, 4, 2, 6};
        table_->sortItems(columns[static_cast<int>(sortMode_)], Qt::AscendingOrder);
    }
    static QIcon toolbarIcon(int kind) {
        QPixmap pixmap(24, 24);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(Qt::black, 1));
        if (kind == 0) {
            painter.setBrush(QColor("#28dc11"));
            painter.drawPolygon(QPolygonF{{10, 3},
                                          {14, 3},
                                          {14, 10},
                                          {21, 10},
                                          {21, 14},
                                          {14, 14},
                                          {14, 21},
                                          {10, 21},
                                          {10, 14},
                                          {3, 14},
                                          {3, 10},
                                          {10, 10}});
        } else if (kind == 1) {
            painter.setBrush(QColor("#0099ff"));
            painter.drawRect(3, 10, 18, 5);
        } else if (kind == 2) {
            painter.setBrush(QColor("#22e5ce"));
            painter.drawPolygon(QPolygonF{{2, 11}, {7, 8}, {11, 13}, {19, 4}, {22, 7}, {11, 20}});
        } else if (kind == 3 || kind == 4) {
            painter.setBrush(kind == 3 ? QColor("#21d426") : QColor("#2367e6"));
            painter.drawPolygon(
                QPolygonF{{5, 9}, {13, 9}, {13, 4}, {22, 12}, {13, 20}, {13, 15}, {5, 15}});
            if (kind == 3) {
                painter.setBrush(QColor("#ae42df"));
                painter.drawRect(2, 10, 3, 4);
            }
        } else if (kind == 5) {
            painter.setBrush(QColor("#ef2834"));
            painter.drawPolygon(QPolygonF{{5, 3},
                                          {12, 9},
                                          {19, 3},
                                          {22, 6},
                                          {15, 12},
                                          {22, 18},
                                          {19, 21},
                                          {12, 15},
                                          {5, 21},
                                          {2, 18},
                                          {9, 12},
                                          {2, 6}});
        } else {
            painter.setBrush(QColor("#f7e51e"));
            painter.drawEllipse(QRectF(10, 2, 4, 4));
            painter.drawRect(QRectF(10, 9, 4, 12));
        }
        return QIcon(pixmap);
    }
    static QString toolbarSettingsPath() {
        const QString overridden = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        if (!overridden.isEmpty())
            return overridden;
        const QString directory =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(directory);
        return QDir(directory).filePath("toolbar.ini");
    }
    void recordVisitedPath(const QString &path) {
        if (path.isEmpty())
            return;
        const int existing = pathCombo_->findText(path, Qt::MatchExactly);
        if (existing >= 0)
            pathCombo_->removeItem(existing);
        pathCombo_->insertItem(0, path);
        while (pathCombo_->count() > 20)
            pathCombo_->removeItem(pathCombo_->count() - 1);
        QStringList paths;
        for (int index = 0; index < pathCombo_->count(); ++index)
            paths << pathCombo_->itemText(index);
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        settings.setValue("history/paths", paths);
    }
    void showFoldersHistory() {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        QDialog dialog(this);
        dialog.setObjectName("foldersHistoryDialog");
        dialog.setWindowTitle("История папок");
        dialog.resize(540, 360);
        auto *layout = new QVBoxLayout(&dialog);
        auto *list = new QListWidget(&dialog);
        list->setObjectName("foldersHistoryList");
        for (int index = 0; index < pathCombo_->count(); ++index)
            list->addItem(pathCombo_->itemText(index));
        if (list->count() > 0)
            list->setCurrentRow(0);
        layout->addWidget(list);
        auto *remove = new QPushButton("Удалить", &dialog);
        remove->setObjectName("removeHistoryEntryButton");
        layout->addWidget(remove);
        connect(remove, &QPushButton::clicked, &dialog,
                [list] { delete list->takeItem(list->currentRow()); });
        auto *buttons =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(list, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QString selected = list->currentItem() ? list->currentItem()->text() : QString();
        const QString current = path_->text();
        pathCombo_->clear();
        for (int index = 0; index < list->count(); ++index)
            pathCombo_->addItem(list->item(index)->text());
        path_->setText(current);
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        QStringList paths;
        for (int index = 0; index < pathCombo_->count(); ++index)
            paths << pathCombo_->itemText(index);
        settings.setValue("history/paths", paths);
        if (!selected.isEmpty())
            navigateTo(selected);
        else
            refreshActions();
    }
    struct Bookmark {
        QString path;
        QString folder;
    };
    void refreshBookmarks() {
        for (int index = 0; index < 10; ++index) {
            const Bookmark &bookmark = bookmarks_[index];
            const QString label =
                bookmark.path.isEmpty()
                    ? QString::number(index) + QStringLiteral(" — пусто")
                    : QString::number(index) + QStringLiteral(" — ") + bookmark.path +
                          (bookmark.folder.isEmpty() ? QString()
                                                     : QStringLiteral(" / ") + bookmark.folder);
            openBookmarks_[index]->setText(label);
            openBookmarks_[index]->setToolTip(label);
            openBookmarks_[index]->setEnabled(
                !bookmark.path.isEmpty() && state_ != State::Loading && state_ != State::Operating);
            saveBookmarks_[index]->setEnabled(state_ != State::Loading &&
                                              state_ != State::Operating);
        }
    }
    void saveBookmark(int index) {
        if (state_ != State::Filesystem && state_ != State::Viewing)
            return;
        Bookmark &bookmark = bookmarks_[index];
        bookmark.path = state_ == State::Filesystem ? directory_ : archive_;
        bookmark.folder = state_ == State::Viewing ? archiveFolder_ : QString();
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        const QString key = QStringLiteral("favorites/%1/").arg(index);
        settings.setValue(key + "path", bookmark.path);
        settings.setValue(key + "folder", bookmark.folder);
        refreshBookmarks();
    }
    void openBookmark(int index) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        const Bookmark bookmark = bookmarks_[index];
        if (bookmark.path.isEmpty())
            return;
        const QFileInfo info(bookmark.path);
        if (bookmark.folder.isEmpty() && info.isDir()) {
            showDirectory(bookmark.path, true);
            return;
        }
        if (!info.isFile() || (info.suffix().compare("7z", Qt::CaseInsensitive) != 0 &&
                               info.suffix().compare("zip", Qt::CaseInsensitive) != 0)) {
            statusBar()->showMessage("Путь закладки недоступен: " + bookmark.path);
            return;
        }
        if (state_ == State::Viewing && archive_ == info.absoluteFilePath()) {
            if (bookmark.folder.isEmpty() || archiveFolderExists(bookmark.folder))
                showArchiveFolder(bookmark.folder);
            else
                statusBar()->showMessage("Папка закладки отсутствует: " + bookmark.folder);
            return;
        }
        pendingBookmarkFolder_ = bookmark.folder;
        openArchive(bookmark.path);
    }
    bool archiveFolderExists(const QString &folder) const {
        for (const ArchiveEntry &entry : entries_) {
            QString path = entry.path;
            if (path.endsWith('/'))
                path.chop(1);
            if ((path == folder && entry.directory) || path.startsWith(folder + '/'))
                return true;
        }
        return false;
    }
    void updateToolbar() {
        auto *toolbar = findChild<QToolBar *>("archiveToolbar");
        const bool archive = archiveToolbarOption_->isChecked();
        const bool standard = standardToolbarOption_->isChecked();
        for (QAction *action : {toolbarAdd_, extract_, test_})
            toolbar->widgetForAction(action)->setVisible(archive);
        for (QAction *action : {copy_, move_, remove_, infoAction_})
            toolbar->widgetForAction(action)->setVisible(standard);
        toolbar->widgetForAction(toolbarGroupSeparator_)->setVisible(archive && standard);
        toolbar->setVisible(archive || standard);
        toolbar->setIconSize(largeButtonsOption_->isChecked() ? QSize(48, 36) : QSize(24, 24));
        toolbar->setToolButtonStyle(buttonTextOption_->isChecked() ? Qt::ToolButtonTextUnderIcon
                                                                   : Qt::ToolButtonIconOnly);
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        settings.setValue("toolbar/archive", archive);
        settings.setValue("toolbar/standard", standard);
        settings.setValue("toolbar/large", largeButtonsOption_->isChecked());
        settings.setValue("toolbar/text", buttonTextOption_->isChecked());
    }
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == table_->viewport() &&
            (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove)) {
            auto *drag = static_cast<QDragMoveEvent *>(event);
            if ((state_ == State::Filesystem || (state_ == State::Viewing && !info_.encrypted)) &&
                !drag->mimeData()->urls().isEmpty() && !backend_->busy()) {
                drag->setDropAction(Qt::CopyAction);
                drag->accept();
                return true;
            }
        }
        if (watched == table_->viewport() && event->type() == QEvent::Drop) {
            auto *drop = static_cast<QDropEvent *>(event);
            QStringList paths;
            for (const QUrl &url : drop->mimeData()->urls()) {
                if (!url.isLocalFile())
                    return true;
                paths << url.toLocalFile();
            }
            if (paths.isEmpty())
                return true;
            if (state_ == State::Filesystem)
                transferFiles(paths, directory_, false);
            else if (state_ == State::Viewing && !info_.encrypted)
                addSources(paths);
            drop->setDropAction(Qt::CopyAction);
            drop->accept();
            return true;
        }
        if ((watched == table_ || watched == table_->viewport()) &&
            event->type() == QEvent::KeyPress) {
            auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 &&
                key->modifiers() == Qt::AltModifier) {
                openBookmark(key->key() - Qt::Key_0);
                return true;
            }
            if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 &&
                key->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
                saveBookmark(key->key() - Qt::Key_0);
                return true;
            }
            if (!(key->modifiers() &
                  (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier))) {
                if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) &&
                    openItem_->isEnabled()) {
                    openItem_->trigger();
                    return true;
                }
                if (key->key() == Qt::Key_Backspace && parent_->isEnabled()) {
                    parent_->trigger();
                    return true;
                }
            }
        }
        return QMainWindow::eventFilter(watched, event);
    }
    void showDirectory(const QString &path, bool remember) {
        const QFileInfo info(path);
        if (!info.isDir()) {
            statusBar()->showMessage("Папка не найдена: " + path);
            path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
            return;
        }
        const QString next = info.canonicalFilePath();
        if (remember && !directory_.isEmpty() && directory_ != next)
            history_.append(directory_);
        directory_ = next;
        recordVisitedPath(directory_);
        archive_.clear();
        archivePassword_.clear();
        archiveFolder_.clear();
        pendingBookmarkFolder_.clear();
        refreshing_ = false;
        refreshFolder_.clear();
        refreshSelection_.clear();
        entries_.clear();
        table_->clearSelection();
        table_->setSortingEnabled(false);
        table_->setRowCount(0);
        path_->setText(directory_);
        const QFileInfoList files = QDir(directory_)
                                        .entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot |
                                                           QDir::Hidden | QDir::System,
                                                       QDir::NoSort);
        for (const QFileInfo &file : files) {
            const int row = table_->rowCount();
            table_->insertRow(row);
            const QStringList values = {file.fileName(),
                                        file.absoluteFilePath(),
                                        file.isDir() ? QString() : QString::number(file.size()),
                                        QString(),
                                        file.lastModified().toString(Qt::ISODate),
                                        file.isDir() ? QString() : file.suffix().toCaseFolded(),
                                        QString::number(row)};
            for (int col = 0; col < values.size(); ++col) {
                auto *item = new ArchiveTableItem(values[col]);
                item->setData(Qt::UserRole + 1, file.isDir());
                item->setData(Qt::UserRole + 3, file.fileName());
                if (col == 0)
                    item->setIcon(QApplication::style()->standardIcon(
                        file.isDir() ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
                if (col == 2 && !file.isDir())
                    item->setData(Qt::UserRole + 2, file.size());
                if (col == 6)
                    item->setData(Qt::UserRole + 2, row);
                if (col == 2 || col == 3)
                    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                table_->setItem(row, col, item);
            }
        }
        table_->setSortingEnabled(true);
        applySortMode();
        static_cast<FileTable *>(table_)->filesystem = true;
        table_->setColumnHidden(3, true);
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(files.size()));
    }
    QStringList selectedPaths() const {
        QStringList paths;
        for (const QModelIndex &index : table_->selectionModel()->selectedRows())
            paths << table_->item(index.row(), 1)->text();
        return paths;
    }
    void selectPaths(const QStringList &paths) {
        table_->clearSelection();
        for (int row = 0; row < table_->rowCount(); ++row) {
            if (paths.contains(table_->item(row, 1)->text()))
                table_->selectionModel()->select(table_->model()->index(row, 0),
                                                 QItemSelectionModel::Select |
                                                     QItemSelectionModel::Rows);
        }
    }
    void refreshCurrent() {
        if (state_ == State::Filesystem) {
            const QStringList selection = selectedPaths();
            showDirectory(directory_, false);
            selectPaths(selection);
        } else if (state_ == State::Viewing) {
            refreshing_ = true;
            refreshFolder_ = archiveFolder_;
            refreshSelection_ = selectedPaths();
            setState(State::Loading);
            begin("Обновление архива", archive_);
            backend_->list(archive_);
        }
    }
    void createFolder() {
        if (state_ != State::Filesystem)
            return;
        bool accepted = false;
        const QString name = QInputDialog::getText(this, "Создать папку",
                                                   "Имя папки:", QLineEdit::Normal, {}, &accepted);
        if (!accepted)
            return;
        const QString target = QDir(directory_).filePath(name);
        if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
            name.contains('\\') || name.contains(QRegularExpression("[<>:\"|?*]")) ||
            name.endsWith(' ') || name.endsWith('.') || QFileInfo::exists(target) ||
            QFileInfo(target).isSymLink() || !QDir(directory_).mkdir(name)) {
            QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось создать папку.");
            return;
        }
        showDirectory(directory_, false);
        selectPaths({target});
    }
    void createFile() {
        if (state_ != State::Filesystem)
            return;
        ComboDialog dialog("Создать файл", "Имя файла:", "Новый файл", this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QString name = dialog.value();
        const QString target = QDir(directory_).filePath(name);
        if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
            name.contains('\\') || name.contains(QRegularExpression("[<>:\"|?*]")) ||
            name.endsWith(' ') || name.endsWith('.') || QFileInfo::exists(target) ||
            QFileInfo(target).isSymLink()) {
            QMessageBox::warning(this, "Создать файл", "Недопустимое имя или файл уже существует.");
            return;
        }
        QFile file(target);
        if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            QMessageBox::warning(this, "Создать файл", "Не удалось создать файл.");
            return;
        }
        file.close();
        showDirectory(directory_, false);
        selectPaths({target});
    }
    void selectByMask(bool select) {
        if (state_ != State::Filesystem && state_ != State::Viewing)
            return;
        ComboDialog dialog(select ? "Выделить" : "Снять выделение", "Маска:", "*", this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QRegularExpression pattern(
            QRegularExpression::wildcardToRegularExpression(dialog.value()),
            QRegularExpression::CaseInsensitiveOption);
        for (int row = 0; row < table_->rowCount(); ++row) {
            if (pattern.match(table_->item(row, 0)->text()).hasMatch())
                table_->selectionModel()->select(
                    table_->model()->index(row, 0),
                    (select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
                        QItemSelectionModel::Rows);
        }
    }
    void selectByType(bool select) {
        if (state_ != State::Filesystem && state_ != State::Viewing)
            return;
        const int current = table_->currentRow();
        if (current < 0)
            return;
        const auto *focused = table_->item(current, 0);
        const bool folder = focused->data(Qt::UserRole + 1).toBool();
        const QString name = focused->text();
        const int dot = name.lastIndexOf('.');
        const QString extension = dot < 0 ? QString() : name.mid(dot);
        for (int row = 0; row < table_->rowCount(); ++row) {
            const auto *item = table_->item(row, 0);
            const bool sameType =
                folder ? item->data(Qt::UserRole + 1).toBool()
                       : !item->data(Qt::UserRole + 1).toBool() &&
                             (extension.isEmpty()
                                  ? !item->text().contains('.')
                                  : item->text().endsWith(extension, Qt::CaseInsensitive));
            if (sameType)
                table_->selectionModel()->select(
                    table_->model()->index(row, 0),
                    (select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
                        QItemSelectionModel::Rows);
        }
    }
    void navigateTo(const QString &path) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (state_ == State::Viewing && path == archiveDisplayPath())
            return;
        const QFileInfo info(path);
        if (info.isDir())
            showDirectory(path, true);
        else if (info.isFile() && (info.suffix().compare("7z", Qt::CaseInsensitive) == 0 ||
                                   info.suffix().compare("zip", Qt::CaseInsensitive) == 0))
            openArchive(path);
        else {
            statusBar()->showMessage("Путь не найден или тип файла не поддерживается: " + path);
            path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
        }
    }
    void activateRow(int row) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (row < 0 || row >= table_->rowCount())
            return;
        if (state_ == State::Filesystem)
            navigateTo(table_->item(row, 1)->text());
        else if (state_ == State::Viewing && table_->item(row, 0)->data(Qt::UserRole + 1).toBool())
            showArchiveFolder(table_->item(row, 0)->data(Qt::UserRole).toString());
    }
    void goBack() {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (state_ == State::Viewing) {
            if (archiveFolder_.isEmpty())
                showDirectory(returnDirectory_, false);
            else
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
        return archive_ +
               (archiveFolder_.isEmpty() ? QString() : QStringLiteral(" / ") + archiveFolder_);
    }
    void showArchiveFolder(const QString &folder) {
        archiveFolder_ = folder;
        recordVisitedPath(archive_);
        static_cast<FileTable *>(table_)->filesystem = false;
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        table_->setColumnHidden(3, !settings.value("columns/3", true).toBool());
        QMap<QString, ArchiveEntry> children;
        QMap<QString, int> sourceOrder;
        const QString prefix = folder.isEmpty() ? QString() : folder + '/';
        for (const ArchiveEntry &entry : entries_) {
            QString path = entry.path;
            if (path.endsWith('/'))
                path.chop(1);
            if (!path.startsWith(prefix) || path == folder)
                continue;
            const QString relative = path.mid(prefix.size());
            const int slash = relative.indexOf('/');
            const QString name = slash < 0 ? relative : relative.left(slash);
            if (name.isEmpty())
                continue;
            if (!sourceOrder.contains(name))
                sourceOrder.insert(name, sourceOrder.size());
            const QString childPath = prefix + name;
            if (slash >= 0) {
                if (!children.contains(name)) {
                    ArchiveEntry implicit;
                    implicit.path = childPath;
                    implicit.directory = true;
                    children.insert(name, implicit);
                }
            } else {
                ArchiveEntry visible = entry;
                visible.path = childPath;
                children.insert(name, visible);
            }
        }
        table_->setSortingEnabled(false);
        table_->clearSelection();
        table_->setRowCount(0);
        for (auto it = children.cbegin(); it != children.cend(); ++it) {
            const ArchiveEntry &entry = it.value();
            const int row = table_->rowCount();
            table_->insertRow(row);
            const QStringList values = {
                it.key(),
                entry.path,
                entry.directory ? QString() : QString::number(entry.size),
                entry.directory ? QString() : QString::number(entry.packedSize),
                entry.modified,
                entry.directory ? QString() : QFileInfo(it.key()).suffix().toCaseFolded(),
                QString::number(sourceOrder.value(it.key()))};
            for (int col = 0; col < values.size(); ++col) {
                auto *item = new ArchiveTableItem(values[col]);
                item->setData(Qt::UserRole, entry.path);
                item->setData(Qt::UserRole + 1, entry.directory);
                item->setData(Qt::UserRole + 3, it.key());
                item->setToolTip(entry.path);
                if (col == 0)
                    item->setIcon(QApplication::style()->standardIcon(
                        entry.directory ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
                if (!entry.directory && (col == 2 || col == 3))
                    item->setData(Qt::UserRole + 2, col == 2 ? entry.size : entry.packedSize);
                if (col == 6)
                    item->setData(Qt::UserRole + 2, sourceOrder.value(it.key()));
                if (col == 2 || col == 3)
                    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                table_->setItem(row, col, item);
            }
        }
        table_->setSortingEnabled(true);
        applySortMode();
        path_->setText(archiveDisplayPath());
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(children.size()));
        refreshActions();
    }
    void setState(State state) {
        state_ = state;
        refreshActions();
    }
    void refreshActions() {
        const bool idle = state_ != State::Loading && state_ != State::Operating;
        for (QAction *action : sortActions_)
            action->setEnabled(idle);
        open_->setEnabled(idle);
        create_->setEnabled(idle && state_ == State::Filesystem);
        add_->setEnabled(idle && state_ == State::Viewing && !info_.encrypted);
        toolbarAdd_->setEnabled(create_->isEnabled() || add_->isEnabled());
        extract_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty());
        test_->setEnabled(idle && state_ == State::Viewing);
        infoAction_->setEnabled(idle && state_ == State::Viewing);
        selected_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty() &&
                              !table_->selectionModel()->selectedRows().isEmpty());
        back_->setEnabled(idle && (state_ == State::Viewing || !history_.isEmpty()));
        up_->setEnabled(idle && state_ == State::Viewing && !archiveFolder_.isEmpty());
        bool canOpenItem = false;
        const int row = table_->currentRow();
        if (idle && row >= 0 && row < table_->rowCount()) {
            if (state_ == State::Viewing)
                canOpenItem = table_->item(row, 0)->data(Qt::UserRole + 1).toBool();
            else {
                const QFileInfo info(table_->item(row, 1)->text());
                canOpenItem =
                    info.isDir() ||
                    (info.isFile() && (info.suffix().compare("7z", Qt::CaseInsensitive) == 0 ||
                                       info.suffix().compare("zip", Qt::CaseInsensitive) == 0));
            }
        }
        openItem_->setEnabled(canOpenItem);
        parent_->setEnabled(idle && (state_ == State::Viewing ||
                                     (state_ == State::Filesystem && QDir(directory_).cdUp())));
        selectAll_->setEnabled(idle && table_->rowCount() > 0);
        const bool selected = !table_->selectionModel()->selectedRows().isEmpty();
        deselectAll_->setEnabled(idle && selected);
        invertSelection_->setEnabled(idle && table_->rowCount() > 0);
        refresh_->setEnabled(idle);
        foldersHistory_->setEnabled(idle && pathCombo_->count() > 0);
        refreshBookmarks();
        createFolder_->setEnabled(idle && state_ == State::Filesystem);
        createFile_->setEnabled(idle && state_ == State::Filesystem);
        selectMask_->setEnabled(idle && table_->rowCount() > 0);
        deselectMask_->setEnabled(idle && table_->rowCount() > 0);
        selectType_->setEnabled(idle && table_->currentRow() >= 0);
        deselectType_->setEnabled(idle && table_->currentRow() >= 0);
        copy_->setEnabled(idle && selected);
        move_->setEnabled(idle && state_ == State::Filesystem && selected);
        remove_->setEnabled(
            idle && selected &&
            (state_ == State::Filesystem || (state_ == State::Viewing && !info_.encrypted)));
        const bool one = table_->selectionModel()->selectedRows().size() == 1;
        rename_->setEnabled(
            idle && one &&
            (state_ == State::Filesystem ||
             (state_ == State::Viewing && !info_.encrypted &&
              !table_->item(table_->selectionModel()->selectedRows().first().row(), 0)
                   ->data(Qt::UserRole + 1)
                   .toBool())));
        properties_->setEnabled(idle && one);
        table_->setEnabled(idle);
        table_->setDragEnabled(idle && state_ == State::Filesystem);
        path_->setReadOnly(!idle);
    }
    void appendJournal(const QString &line) {
        journalText_ += line + '\n';
        if (journalView_)
            journalView_->appendPlainText(line);
    }
    void showJournal() {
        if (!journalDialog_) {
            journalDialog_ = new QDialog(this);
            journalDialog_->setObjectName("operationJournalDialog");
            journalDialog_->setWindowTitle("Журнал последней операции");
            journalDialog_->resize(700, 440);
            auto *layout = new QVBoxLayout(journalDialog_);
            journalView_ = new QPlainTextEdit(journalDialog_);
            journalView_->setObjectName("operationJournalText");
            journalView_->setReadOnly(true);
            journalView_->setPlainText(journalText_);
            layout->addWidget(journalView_);
        }
        journalDialog_->show();
        journalDialog_->raise();
        journalDialog_->activateWindow();
    }
    void showInfo() {
        if (state_ != State::Viewing)
            return;
        if (infoDialog_)
            infoDialog_->close();
        auto *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        infoDialog_ = dialog;
        dialog->setObjectName("archiveInfoDialog");
        dialog->setWindowTitle("Информация об архиве");
        auto *layout = new QVBoxLayout(dialog);
        auto *label = new QLabel(dialog);
        label->setObjectName("archiveInfoText");
        label->setText(
            QStringLiteral(
                "Формат: %1\nРазмер архива: %2\nФайлов: %3\nПапок: %4\nРазмер файлов: %5 байт")
                .arg(info_.format.isEmpty() ? QStringLiteral("не указан 7zz") : info_.format)
                .arg(info_.physicalSize < 0
                         ? QStringLiteral("не указан 7zz")
                         : QString::number(info_.physicalSize) + QStringLiteral(" байт"))
                .arg(info_.files)
                .arg(info_.folders)
                .arg(info_.unpackedSize));
        layout->addWidget(label);
        dialog->show();
    }
    void showSelectedProperties() {
        if (!properties_->isEnabled())
            return;
        const int row = table_->selectionModel()->selectedRows().first().row();
        const QTableWidgetItem *nameItem = table_->item(row, 0);
        const bool directory = nameItem->data(Qt::UserRole + 1).toBool();
        const QString path = state_ == State::Filesystem ? table_->item(row, 1)->text()
                                                         : nameItem->data(Qt::UserRole).toString();
        QDialog dialog(this);
        dialog.setObjectName("selectedPropertiesDialog");
        dialog.setWindowTitle("Свойства: " + nameItem->text());
        auto *layout = new QVBoxLayout(&dialog);
        auto *fields = new QFormLayout;
        fields->addRow("Имя:", new QLabel(nameItem->text(), &dialog));
        fields->addRow("Путь:", new QLabel(path, &dialog));
        fields->addRow("Тип:", new QLabel(directory ? "Папка" : "Файл", &dialog));
        if (state_ == State::Filesystem) {
            const QFileInfo info(path);
            if (!directory)
                fields->addRow("Размер:", new QLabel(QString::number(info.size()), &dialog));
            fields->addRow("Изменён:",
                           new QLabel(info.lastModified().toString(Qt::ISODate), &dialog));
        } else {
            if (!directory) {
                fields->addRow("Размер:", new QLabel(table_->item(row, 2)->text(), &dialog));
                fields->addRow("Сжатый:", new QLabel(table_->item(row, 3)->text(), &dialog));
            }
            if (!table_->item(row, 4)->text().isEmpty())
                fields->addRow("Изменён:", new QLabel(table_->item(row, 4)->text(), &dialog));
        }
        layout->addLayout(fields);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);
        dialog.exec();
    }
    void begin(const QString &label, const QString &detail) {
        operationName_ = label;
        currentDetail_ = detail;
        journalText_.clear();
        if (journalView_)
            journalView_->clear();
        appendJournal(label + ": " + detail);
        journalAction_->setEnabled(true);
        progress_ = new QProgressDialog(label + "\n" + detail, "Отмена", 0, 100, this);
        progress_->setWindowModality(Qt::NonModal);
        progress_->setMinimumDuration(0);
        progress_->setValue(0);
        connect(progress_, &QProgressDialog::canceled, backend_, &ArchiveBackend::cancel);
    }
    void openDialog() {
        const QString file =
            QFileDialog::getOpenFileName(this, "Открыть архив", {}, "Архивы (*.7z *.zip)");
        if (!file.isEmpty())
            openArchive(file);
    }
    void extract(bool onlySelected) {
        if (state_ != State::Viewing || entries_.isEmpty())
            return;
        if (onlySelected && table_->selectionModel()->selectedRows().isEmpty())
            return;
        QDialog dialog(this);
        dialog.setWindowTitle(onlySelected ? "Извлечь выбранное" : "Извлечь архив");
        auto *layout = new QVBoxLayout(&dialog);
        layout->addWidget(new QLabel("Папка назначения:", &dialog));
        auto *destination = new QLineEdit(returnDirectory_, &dialog);
        destination->setObjectName("extractDestination");
        layout->addWidget(destination);
        auto *browse = new QPushButton("Обзор…", &dialog);
        layout->addWidget(browse);
        connect(browse, &QPushButton::clicked, &dialog, [this, destination] {
            const QString path = QFileDialog::getExistingDirectory(this, "Папка для извлечения",
                                                                   destination->text());
            if (!path.isEmpty())
                destination->setText(path);
        });
        layout->addWidget(new QLabel("Пути внутри архива сохраняются. Существующие файлы не "
                                     "перезаписываются.",
                                     &dialog));
        auto *buttons =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);
        if (dialog.exec() == QDialog::Accepted)
            extractTo(destination->text(), onlySelected);
    }
    void createDialog() {
        if (state_ != State::Filesystem)
            return;
        QStringList selectedPaths;
        for (const QModelIndex &index : table_->selectionModel()->selectedRows())
            selectedPaths << table_->item(index.row(), 1)->text();
        CreateArchiveDialog dialog(directory_, selectedPaths, this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        createArchive(dialog.target(), dialog.sources(), dialog.password(),
                      dialog.encryptHeaders());
    }
    void createArchive(const QString &target, const QStringList &sources,
                       const QString &password = {}, bool encryptHeaders = false) {
        if (state_ != State::Filesystem || sources.isEmpty() || target.isEmpty())
            return;
        previousState_ = state_;
        creating_ = true;
        setState(State::Operating);
        begin("Создание архива", target);
        backend_->create(target, sources, password, encryptHeaders);
    }
    void addDialog() {
        if (state_ != State::Viewing)
            return;
        CreateArchiveDialog dialog(returnDirectory_, {}, this, archive_);
        if (dialog.exec() != QDialog::Accepted)
            return;
        if (state_ != State::Viewing || dialog.sources().isEmpty())
            return;
        addSources(dialog.sources());
    }

    QStringList selectedFilesystemPaths() const {
        QStringList paths;
        if (state_ != State::Filesystem)
            return paths;
        for (const QModelIndex &index : table_->selectionModel()->selectedRows())
            paths << table_->item(index.row(), 1)->text();
        return paths;
    }
    static bool copyTree(const QString &source, const QString &target) {
        const QFileInfo info(source);
        if (info.isSymLink() || !info.exists())
            return false;
        if (!info.isDir())
            return QFile::copy(source, target);
        if (!QDir().mkdir(target))
            return false;
        const QFileInfoList children = QDir(source).entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QFileInfo &child : children) {
            if (!copyTree(child.absoluteFilePath(), QDir(target).filePath(child.fileName())))
                return false;
        }
        return true;
    }
    static void removeTree(const QString &path) {
        if (QFileInfo(path).isDir())
            QDir(path).removeRecursively();
        else
            QFile::remove(path);
    }
    void transferFiles(const QStringList &sources, const QString &destination, bool move) {
        if (state_ != State::Filesystem || !QFileInfo(destination).isDir() ||
            QFileInfo(destination).isSymLink() || sources.isEmpty()) {
            QMessageBox::warning(this, "Sevenzip Desktop", "Папка назначения недоступна.");
            return;
        }
        QStringList targets;
        const QString canonicalDestination = QFileInfo(destination).canonicalFilePath();
        for (const QString &source : sources) {
            const QFileInfo info(source);
            const QString target = QDir(destination).filePath(info.fileName());
            if (!info.exists() || info.isSymLink() || QFileInfo::exists(target) ||
                QFileInfo(target).isSymLink() || targets.contains(target) ||
                (info.isDir() &&
                 (canonicalDestination == info.canonicalFilePath() ||
                  canonicalDestination.startsWith(info.canonicalFilePath() + '/')))) {
                QMessageBox::warning(this, "Sevenzip Desktop",
                                     "Источник или назначение недоступны либо имя уже занято.");
                return;
            }
            targets << target;
        }
        QStringList completed;
        for (qsizetype i = 0; i < sources.size(); ++i) {
            const QString stage =
                targets[i] +
                QStringLiteral(".sevenzip-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
            bool ok = move ? QDir().rename(sources[i], stage) : copyTree(sources[i], stage);
            if (ok)
                ok = QDir().rename(stage, targets[i]);
            if (!ok) {
                if (move && QFileInfo::exists(stage))
                    QDir().rename(stage, sources[i]);
                else
                    removeTree(stage);
                for (qsizetype j = completed.size(); j > 0; --j) {
                    if (move)
                        QDir().rename(completed[j - 1], sources[j - 1]);
                    else
                        removeTree(completed[j - 1]);
                }
                QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось выполнить операцию.");
                showDirectory(directory_, false);
                return;
            }
            completed << targets[i];
        }
        showDirectory(directory_, false);
        statusBar()->showMessage(move ? "Объекты перемещены." : "Объекты скопированы.");
    }
    void copySelected() {
        if (state_ == State::Viewing) {
            extract(true);
            return;
        }
        if (state_ != State::Filesystem || selectedFilesystemPaths().isEmpty())
            return;
        transferSelected(false);
    }
    void moveSelected() {
        if (state_ != State::Filesystem || selectedFilesystemPaths().isEmpty())
            return;
        transferSelected(true);
    }
    void transferSelected(bool move) {
        const QStringList sources = selectedFilesystemPaths();
        if (state_ != State::Filesystem || sources.isEmpty())
            return;
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        CopyDialog dialog(move, directory_, sources, settings.value("copy/history").toStringList(),
                          this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QString destination = dialog.destination();
        transferFiles(sources, destination, move);
        if (!QFileInfo(destination).isDir())
            return;
        QStringList history = settings.value("copy/history").toStringList();
        history.removeAll(destination);
        history.prepend(destination);
        while (history.size() > 20)
            history.removeLast();
        settings.setValue("copy/history", history);
    }
    void deleteSelected() {
        if (state_ == State::Viewing) {
            const QStringList paths = selectedArchivePaths();
            if (info_.encrypted || paths.isEmpty() ||
                QMessageBox::question(this, "Удалить",
                                      "Удалить выбранные объекты из открытого архива?") !=
                    QMessageBox::Yes)
                return;
            previousState_ = state_;
            updatingArchive_ = true;
            setState(State::Operating);
            begin("Удаление из архива", archive_);
            backend_->remove(archive_, paths);
            return;
        }
        const QStringList paths = selectedFilesystemPaths();
        if (paths.isEmpty())
            return;
        if (QMessageBox::question(this, "Удалить",
                                  "Удалить выбранные объекты из файловой системы?") !=
            QMessageBox::Yes)
            return;
        for (const QString &path : paths) {
            if (QFileInfo(path).isSymLink() || !QFileInfo(path).exists()) {
                QMessageBox::warning(this, "Sevenzip Desktop", "Объект недоступен.");
                break;
            }
            bool ok =
                QFileInfo(path).isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
            if (!ok) {
                QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось удалить объект.");
                break;
            }
        }
        showDirectory(directory_, false);
    }
    void renameSelected() {
        if (state_ == State::Viewing) {
            if (!rename_->isEnabled())
                return;
            const QString oldPath =
                table_->item(table_->selectionModel()->selectedRows().first().row(), 0)
                    ->data(Qt::UserRole)
                    .toString();
            bool accepted = false;
            const QString name =
                QInputDialog::getText(this, "Переименовать", "Новое имя:", QLineEdit::Normal,
                                      oldPath.section('/', -1), &accepted);
            if (!accepted)
                return;
            if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
                name.contains('\\')) {
                QMessageBox::warning(this, "Sevenzip Desktop", "Недопустимое имя файла.");
                return;
            }
            const int slash = oldPath.lastIndexOf('/');
            const QString newPath = (slash < 0 ? QString() : oldPath.left(slash + 1)) + name;
            previousState_ = state_;
            updatingArchive_ = true;
            setState(State::Operating);
            begin("Переименование в архиве", archive_);
            backend_->renameEntry(archive_, oldPath, newPath);
            return;
        }
        const QStringList paths = selectedFilesystemPaths();
        if (paths.size() != 1)
            return;
        const QFileInfo source(paths.first());
        bool accepted = false;
        const QString name = QInputDialog::getText(
            this, "Переименовать", "Новое имя:", QLineEdit::Normal, source.fileName(), &accepted);
        if (!accepted)
            return;
        if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
            name.contains('\\') || source.isSymLink() ||
            QFileInfo::exists(QDir(directory_).filePath(name)) ||
            !QDir().rename(paths.first(), QDir(directory_).filePath(name)))
            QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось переименовать объект.");
        showDirectory(directory_, false);
    }
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
    QString archive_;
    QString archivePassword_;
    QString archiveFolder_;
    QString pendingBookmarkFolder_;
    Bookmark bookmarks_[10];
    QAction *saveBookmarks_[10]{};
    QAction *openBookmarks_[10]{};
    State state_ = State::Filesystem;
    SortMode sortMode_ = SortMode::Name;
    QAction *sortActions_[5]{};
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
    QAction *parent_ = nullptr;
    QAction *selectAll_ = nullptr;
    QAction *copy_ = nullptr;
    QAction *move_ = nullptr;
    QAction *remove_ = nullptr;
    QAction *rename_ = nullptr;
    QAction *properties_ = nullptr;
    QAction *createFolder_ = nullptr;
    QAction *createFile_ = nullptr;
    QAction *refresh_ = nullptr;
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
