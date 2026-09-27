#include "mainwindow.h"

MainWindow::MainWindow(ArchiveBackend *backend, std::function<bool(const QUrl &)> fileOpener)
    : fileOpener_(fileOpener ? std::move(fileOpener) : QDesktopServices::openUrl) {
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
    openInside_ = menu->addAction("Открыть внутри", this, [this] { openSelectedInside(); });
    openInside_->setObjectName("openInsideAction");
    openInside_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    openOutside_ = menu->addAction("Открыть снаружи", this, [this] { openSelectedOutside(); });
    openOutside_->setObjectName("openOutsideAction");
    openOutside_->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return));
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
    journalAction_ = menu->addAction("Журнал последней операции…", this, [this] { showJournal(); });
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
    split_ = menu->addAction("Разбить файл…", this,
                             [this] { startVolume(VolumeOperation::Mode::Split); });
    split_->setObjectName("splitFileAction");
    combine_ = menu->addAction("Объединить файлы…", this,
                               [this] { startVolume(VolumeOperation::Mode::Combine); });
    combine_->setObjectName("combineFilesAction");
    rename_ = menu->addAction("Переименовать…", this, [this] { renameSelected(); });
    rename_->setObjectName("renameAction");
    viewFile_ = menu->addAction("Просмотреть", this, [this] { viewSelectedFile(false); });
    viewFile_->setObjectName("viewFileAction");
    viewFile_->setShortcut(QKeySequence(Qt::Key_F3));
    editFile_ = menu->addAction("Редактировать", this, [this] { viewSelectedFile(true); });
    editFile_->setObjectName("editFileAction");
    editFile_->setShortcut(QKeySequence(Qt::Key_F4));
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
    for (const QString &title : {"Пароль", "Изменить архив", "Интеграция с файловым менеджером"})
        menu->addAction(title)->setEnabled(false);
    open_ = open;
    auto *editMenu = menuBar()->addMenu("Правка");
    auto *viewMenu = menuBar()->addMenu("Вид");
    auto *viewGroup = new QActionGroup(this);
    const QStringList viewLabels = {"Крупные значки", "Мелкие значки", "Список", "Подробности"};
    for (int index = 0; index < viewLabels.size(); ++index) {
        QAction *action = viewMenu->addAction(viewLabels[index]);
        action->setObjectName(QStringLiteral("viewMode%1Action").arg(index));
        action->setCheckable(true);
        action->setActionGroup(viewGroup);
        action->setShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + index)));
        viewActions_[index] = action;
        connect(action, &QAction::triggered, this, [this, index] { setViewMode(index); });
    }
    viewMenu->addSeparator();
    auto *favoritesMenu = menuBar()->addMenu("Избранное");
    auto *toolsMenu = menuBar()->addMenu("Сервис");
    settingsAction_ = toolsMenu->addAction("Настройки…", this, [this] { showSettings(); });
    settingsAction_->setObjectName("settingsAction");
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
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    fileListSettings_ = {settings.value("list/realFileIcons", false).toBool(),
                         settings.value("list/grid", false).toBool(),
                         settings.value("list/singleClick", false).toBool()};
    flatDisk_ = settings.value("view/flatDisk", false).toBool();
    flatArchive_ = settings.value("view/flatArchive", false).toBool();
    flatView_ = viewMenu->addAction("Плоский вид");
    flatView_->setObjectName("flatViewAction");
    flatView_->setCheckable(true);
    flatView_->setChecked(flatDisk_);
    connect(flatView_, &QAction::toggled, this, [this](bool enabled) {
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        const QStringList selection = selectedPaths();
        if (state_ == State::Viewing) {
            flatArchive_ = enabled;
            settings.setValue("view/flatArchive", enabled);
            showArchiveFolder(archiveFolder_);
        } else if (state_ == State::Filesystem) {
            flatDisk_ = enabled;
            settings.setValue("view/flatDisk", enabled);
            showDirectory(directory_, false);
            if (scanningDisk_)
                scanSelection_ = selection;
        }
        if (!scanningDisk_)
            selectPaths(selection);
    });
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
    archiveToolbarOption_ = addToolbarOption("Архивная панель", "archiveToolbarOption",
                                             settings.value("toolbar/archive", true).toBool());
    standardToolbarOption_ = addToolbarOption("Стандартная панель", "standardToolbarOption",
                                              settings.value("toolbar/standard", true).toBool());
    largeButtonsOption_ = addToolbarOption("Крупные значки", "largeButtonsOption",
                                           settings.value("toolbar/large", false).toBool());
    buttonTextOption_ = addToolbarOption("Подписи кнопок", "buttonTextOption",
                                         settings.value("toolbar/text", true).toBool());
    updateToolbar();
    refresh_ = viewMenu->addAction("Обновить", this, [this] { refreshCurrent(); });
    refresh_->setObjectName("refreshAction");
    refresh_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    foldersHistory_ = viewMenu->addAction("История папок…", this, [this] { showFoldersHistory(); });
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
    table_->setShowGrid(fileListSettings_.grid);
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
                    QAction *option = menu.addAction(table_->horizontalHeaderItem(column)->text());
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
        showItemContextMenu(table_->viewport()->mapToGlobal(pos));
    });
    openItem_ = new QAction(table_);
    openItem_->setObjectName("openItemAction");
    table_->addAction(openItem_);
    connect(openItem_, &QAction::triggered, this, [this] { activateRow(table_->currentRow()); });
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
    selectMask_ = editMenu->addAction("Выделить по маске…", this, [this] { selectByMask(true); });
    selectMask_->setObjectName("selectMaskAction");
    deselectMask_ =
        editMenu->addAction("Снять выделение по маске…", this, [this] { selectByMask(false); });
    deselectMask_->setObjectName("deselectMaskAction");
    selectType_ = editMenu->addAction("Выделить по типу", this, [this] { selectByType(true); });
    selectType_->setObjectName("selectTypeAction");
    deselectType_ =
        editMenu->addAction("Снять выделение по типу", this, [this] { selectByType(false); });
    deselectType_->setObjectName("deselectTypeAction");
    iconList_ = new QListView(central);
    iconList_->setObjectName("iconList");
    iconList_->setModel(table_->model());
    iconList_->setModelColumn(0);
    iconList_->setSelectionModel(table_->selectionModel());
    iconList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    iconList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    iconList_->setUniformItemSizes(true);
    iconList_->setWordWrap(false);
    iconList_->setDragEnabled(true);
    iconList_->setAcceptDrops(true);
    iconList_->viewport()->setAcceptDrops(true);
    iconList_->setContextMenuPolicy(Qt::CustomContextMenu);
    iconList_->installEventFilter(this);
    iconList_->viewport()->installEventFilter(this);
    iconList_->addAction(selectAll_);
    iconList_->addAction(openItem_);
    iconList_->addAction(parent_);
    connect(iconList_, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        if (!fileListSettings_.singleClick)
            activateRow(index.row());
    });
    connect(iconList_, &QListView::clicked, this, [this](const QModelIndex &index) {
        if (fileListSettings_.singleClick)
            activateRow(index.row());
    });
    connect(iconList_, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex index = iconList_->indexAt(pos);
        if (index.isValid() &&
            !iconList_->selectionModel()->isRowSelected(index.row(), QModelIndex()))
            iconList_->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect |
                                                           QItemSelectionModel::Rows);
        showItemContextMenu(iconList_->viewport()->mapToGlobal(pos));
    });
    viewStack_ = new QStackedWidget(central);
    viewStack_->setObjectName("fileViewStack");
    viewStack_->addWidget(table_);
    viewStack_->addWidget(iconList_);
    layout->addWidget(viewStack_);
    int initialView = settings.value("view/mode", 3).toInt();
    if (initialView < 0 || initialView > 3)
        initialView = 3;
    setViewMode(initialView);
    setCentralWidget(central);
    statusBar()->showMessage("Готово");
    connect(path_, &QLineEdit::returnPressed, this, [this] { navigateTo(path_->text()); });
    connect(pathCombo_, &QComboBox::activated, this,
            [this](int index) { navigateTo(pathCombo_->itemText(index)); });
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (!fileListSettings_.singleClick)
            activateRow(row);
    });
    connect(table_, &QTableWidget::cellClicked, this, [this](int row, int) {
        if (fileListSettings_.singleClick)
            activateRow(row);
    });
    filesystemScanner_ = new FilesystemScanner(this);
    connect(filesystemScanner_, &FilesystemScanner::batchReady, this,
            [this](const QFileInfoList &files) {
                for (const QFileInfo &file : files)
                    appendFilesystemEntry(file);
                statusBar()->showMessage(
                    QStringLiteral("Сканирование: %1 объектов…").arg(table_->rowCount()));
            });
    connect(filesystemScanner_, &FilesystemScanner::completed, this, [this] {
        scanningDisk_ = false;
        table_->setSortingEnabled(true);
        applySortMode();
        selectPaths(scanSelection_);
        scanSelection_.clear();
        refreshActions();
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(table_->rowCount()));
    });
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
                {
                    const QSignalBlocker blocked(flatView_);
                    flatView_->setChecked(flatArchive_);
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
    connect(backend_, &ArchiveBackend::progress, this, [this](int percent, const QString &detail) {
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
        if (nestedListing_ && ok)
            nestedListing_ = false;
        if (!ok)
            archivePassword_.clear();
        appendJournal((ok ? QStringLiteral("Результат: ") : QStringLiteral("Ошибка: ")) + message);
        if (progress_) {
            QProgressDialog *completed = progress_;
            progress_ = nullptr;
            completed->disconnect(backend_);
            completed->close();
            completed->deleteLater();
        }
        if (state_ == State::Loading && !ok) {
            if (nestedListing_ && !archiveParents_.empty())
                restoreParentArchive();
            else
                showDirectory(returnDirectory_, false);
        } else if (state_ == State::Operating) {
            if (replacingArchiveEntry_) {
                replacingArchiveEntry_ = false;
                viewTemporary_.reset();
                if (ok) {
                    refreshing_ = true;
                    refreshFolder_ = archiveFolder_;
                    refreshSelection_ = {pendingEditEntry_};
                }
                pendingEditEntry_.clear();
            }
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
        if (pendingView_) {
            const QString file = pendingViewPath_;
            const bool external = pendingExternalOpen_;
            const bool editable = pendingViewEditable_;
            pendingView_ = false;
            pendingExternalOpen_ = false;
            pendingViewEditable_ = false;
            pendingViewPath_.clear();
            if (ok && QFileInfo(file).isFile()) {
                QTimer::singleShot(0, this, [this, file, external, editable] {
                    if (external) {
                        if (openLocalFile(file))
                            openedTemporaries_.push_back(std::move(viewTemporary_));
                        else
                            viewTemporary_.reset();
                    } else
                        openFileViewer(file, editable);
                });
            } else {
                pendingEditEntry_.clear();
                viewTemporary_.reset();
            }
        }
        if (pendingNested_) {
            if (finishNestedExtraction(ok))
                return;
        }
        statusBar()->showMessage(message);
        if (!ok && message != "Операция отменена.")
            QMessageBox::warning(this, "Sevenzip Desktop", message);
    });
}
