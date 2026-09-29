#include "mainwindow.h"

QMenu *MainWindow::createMenusAndToolbar() {
    auto *menu = menuBar()->addMenu("Файл");
    auto *toolbar = addToolBar("Архив");
    toolbar->setObjectName("archiveToolbar");
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setIconSize(QSize(24, 24));
    createFileActions(menu, toolbar);
    createFileExtras(menu, toolbar);
    auto *editMenu = menuBar()->addMenu("Правка");
    auto *viewMenu = menuBar()->addMenu("Вид");
    createViewModes(viewMenu);
    viewMenu->addSeparator();
    auto *favoritesMenu = menuBar()->addMenu("Избранное");
    auto *toolsMenu = menuBar()->addMenu("Сервис");
    settingsAction_ = toolsMenu->addAction("Настройки…");
    settingsAction_->setObjectName("settingsAction");
    createFavoritesMenu(favoritesMenu);
    createViewOptions(viewMenu);
    return editMenu;
}

void MainWindow::createFileActions(QMenu *menu, QToolBar *toolbar) {
    back_ = new QAction("Назад", this);
    back_->setObjectName("backAction");
    up_ = new QAction("Вверх", this);
    up_->setObjectName("upAction");
    open_ = menu->addAction("Открыть…");
    open_->setObjectName("openArchiveAction");
    open_->setShortcut(QKeySequence::Open);
    openInside_ = menu->addAction("Открыть внутри");
    openInside_->setObjectName("openInsideAction");
    openInside_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    openOutside_ = menu->addAction("Открыть снаружи");
    openOutside_->setObjectName("openOutsideAction");
    openOutside_->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return));
    create_ = menu->addAction("Создать архив…");
    create_->setObjectName("createArchiveAction");
    add_ = menu->addAction("Добавить в открытый архив…");
    add_->setObjectName("addArchiveAction");
    toolbarAdd_ = toolbar->addAction("Добавить");
    toolbarAdd_->setObjectName("toolbarAddAction");
    extract_ = toolbar->addAction("Извлечь");
    extract_->setObjectName("extractArchiveAction");
    selected_ = menu->addAction("Извлечь выбранное…");
    selected_->setObjectName("extractSelectedAction");
    journalAction_ = menu->addAction("Журнал последней операции…");
    journalAction_->setObjectName("operationJournalAction");
    journalAction_->setEnabled(false);
    test_ = toolbar->addAction("Тест");
    test_->setObjectName("testArchiveAction");
    copy_ = toolbar->addAction("Копировать");
    copy_->setObjectName("copyAction");
    move_ = toolbar->addAction("Переместить");
    move_->setObjectName("moveAction");
    remove_ = toolbar->addAction("Удалить");
    remove_->setObjectName("deleteAction");
    split_ = menu->addAction("Разбить файл…");
    split_->setObjectName("splitFileAction");
    combine_ = menu->addAction("Объединить файлы…");
    combine_->setObjectName("combineFilesAction");
    rename_ = menu->addAction("Переименовать…");
    rename_->setObjectName("renameAction");
    viewFile_ = menu->addAction("Просмотреть");
    viewFile_->setObjectName("viewFileAction");
    viewFile_->setShortcut(QKeySequence(Qt::Key_F3));
    editFile_ = menu->addAction("Редактировать");
    editFile_->setObjectName("editFileAction");
    editFile_->setShortcut(QKeySequence(Qt::Key_F4));
    properties_ = menu->addAction("Свойства…");
    properties_->setObjectName("selectedPropertiesAction");
    properties_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));
}

void MainWindow::createFileExtras(QMenu *menu, QToolBar *toolbar) {
    auto *hashMenu = menu->addMenu("CRC");
    const QStringList hashLabels = {"CRC-32",   "CRC-64",   "XXH64",   "MD5",
                                    "SHA-1",    "SHA-256",  "SHA-384", "SHA-512",
                                    "SHA3-256", "BLAKE2sp", "*"};
    const QStringList hashMethods = {"CRC32",  "CRC64",  "XXH64",    "MD5",      "SHA1", "SHA256",
                                     "SHA384", "SHA512", "SHA3-256", "BLAKE2sp", "*"};
    for (int index = 0; index < hashLabels.size(); ++index) {
        auto *action = hashMenu->addAction(hashLabels[index]);
        action->setObjectName(QStringLiteral("hash%1Action").arg(hashMethods[index]));
        hashActions_[index] = action;
    }
    createFolder_ = menu->addAction("Создать папку…");
    createFolder_->setObjectName("createFolderAction");
    createFolder_->setShortcut(QKeySequence(Qt::Key_F7));
    createFile_ = menu->addAction("Создать файл…");
    createFile_->setObjectName("createFileAction");
    createFile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    copy_->setShortcut(QKeySequence(Qt::Key_F5));
    move_->setShortcut(QKeySequence(Qt::Key_F6));
    remove_->setShortcut(QKeySequence::Delete);
    rename_->setShortcut(QKeySequence(Qt::Key_F2));
    infoAction_ = toolbar->addAction("Информация");
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
}

void MainWindow::createViewModes(QMenu *viewMenu) {
    auto *viewGroup = new QActionGroup(this);
    const QStringList viewLabels = {"Крупные значки", "Мелкие значки", "Список", "Подробности"};
    for (int index = 0; index < viewLabels.size(); ++index) {
        QAction *action = viewMenu->addAction(viewLabels[index]);
        action->setObjectName(QStringLiteral("viewMode%1Action").arg(index));
        action->setCheckable(true);
        action->setActionGroup(viewGroup);
        action->setShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + index)));
        viewActions_[index] = action;
    }
}

void MainWindow::createFavoritesMenu(QMenu *favoritesMenu) {
    auto *addFavoriteMenu = favoritesMenu->addMenu("Добавить папку в избранное как");
    for (int index = 0; index < 10; ++index) {
        auto *save = addFavoriteMenu->addAction(QString::number(index));
        save->setObjectName(QStringLiteral("saveBookmark%1Action").arg(index));
        auto *open = favoritesMenu->addAction(QString::number(index));
        open->setObjectName(QStringLiteral("openBookmark%1Action").arg(index));
        saveBookmarks_[index] = save;
        openBookmarks_[index] = open;
    }
    refreshBookmarks();
}

void MainWindow::createViewOptions(QMenu *viewMenu) {
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
    }
    sortActions_[0]->setChecked(true);
    viewMenu->addSeparator();
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    fileListSettings_.Load(toolbarSettingsPath());
    flatDisk_ = settings.value("view/flatDisk", false).toBool();
    flatArchive_ = settings.value("view/flatArchive", false).toBool();
    flatView_ = viewMenu->addAction("Плоский вид");
    flatView_->setObjectName("flatViewAction");
    flatView_->setCheckable(true);
    flatView_->setChecked(flatDisk_);
    viewMenu->addSeparator();
    createToolbarOptions(viewMenu);
    root_ = viewMenu->addAction("Открыть корневую папку");
    root_->setObjectName("openRootAction");
    refresh_ = viewMenu->addAction("Обновить");
    refresh_->setObjectName("refreshAction");
    refresh_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    foldersHistory_ = viewMenu->addAction("История папок…");
    foldersHistory_->setObjectName("foldersHistoryAction");
    foldersHistory_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_F12));
}

void MainWindow::applyFlatView(bool enabled) {
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    CSelectedState selection;
    app_.GetFocusedPanel().SaveSelectedState(selection);
    {
        const QSignalBlocker block(flatView_);
        flatView_->setChecked(enabled);
    }
    if (state_ == State::Viewing) {
        flatArchive_ = enabled;
        app_.GetFocusedPanel().SetFlatModeForArc(enabled);
        SaveFlatView(app_.GetFocusedPanelIndex(), enabled, toolbarSettingsPath());
        showArchiveFolder(archiveFolder_);
    } else if (state_ == State::Filesystem) {
        flatDisk_ = enabled;
        app_.GetFocusedPanel().SetFlatModeForDisk(enabled);
        settings.setValue("view/flatDisk", enabled);
        showDirectory(directory_, false);
        if (scanningDisk_)
            scanSelectedState_ = selection;
    }
    if (!scanningDisk_) {
        app_.GetFocusedPanel().RefreshListCtrl(selection);
        refreshActions();
    }
}

void MainWindow::createToolbarOptions(QMenu *viewMenu) {
    auto *toolbarsMenu = viewMenu->addMenu("Панели инструментов");
    auto addToolbarOption = [this, toolbarsMenu](const QString &title, const char *name,
                                                 bool checked) {
        auto *option = toolbarsMenu->addAction(title);
        option->setObjectName(name);
        option->setCheckable(true);
        option->setChecked(checked);
        return option;
    };
    archiveToolbarOption_ =
        addToolbarOption("Архивная панель", "archiveToolbarOption", app_.ShowArchiveToolbar);
    standardToolbarOption_ =
        addToolbarOption("Стандартная панель", "standardToolbarOption", app_.ShowStandardToolbar);
    largeButtonsOption_ =
        addToolbarOption("Крупные значки", "largeButtonsOption", app_.LargeButtons);
    buttonTextOption_ =
        addToolbarOption("Подписи кнопок", "buttonTextOption", app_.ShowButtonsLables);
    updateToolbar();
}
