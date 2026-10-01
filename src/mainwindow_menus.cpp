#include "7zip/UI/FileManager/resource.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "Windows/TimeUtils.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"

QMenu *MainWindow::createMenusAndToolbar() {
    auto *menu = menuBar()->addMenu("Файл");
    menu->setObjectName("fileMenu");
    auto *toolbar = addToolBar("Архив");
    toolbar->setObjectName("archiveToolbar");
    QtBindLocalText(toolbar, "Archive", QStringLiteral("Архив"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setIconSize(QSize(24, 24));
    createFileActions(menu, toolbar);
    createFileExtras(menu, toolbar);
    auto *editMenu = menuBar()->addMenu("Правка");
    editMenu->setObjectName("editMenu");
    auto *viewMenu = menuBar()->addMenu("Вид");
    viewMenu->setObjectName("viewMenu");
    createViewModes(viewMenu);
    viewMenu->addSeparator();
    auto *favoritesMenu = menuBar()->addMenu("Избранное");
    favoritesMenu->setObjectName("favoritesMenu");
    auto *toolsMenu = menuBar()->addMenu("Сервис");
    toolsMenu->setObjectName("toolsMenu");
    settingsAction_ = toolsMenu->addAction("Настройки…");
    settingsAction_->setObjectName("settingsAction");
    menuBar()->addMenu("Справка")->setObjectName("helpMenu");
    createFavoritesMenu(favoritesMenu);
    createViewOptions(viewMenu);
    return editMenu;
}

void MainWindow::createFileActions(QMenu *menu, QToolBar *toolbar) {
    back_ = new QAction("Назад", this);
    back_->setObjectName("backAction");
    QtBindLocalText(back_, "Back", QStringLiteral("Назад"));
    up_ = new QAction("Вверх", this);
    up_->setObjectName("upAction");
    openItem_ = new QAction("Открыть", this);
    openItem_->setObjectName("openItemAction");
    open_ = menu->addAction("Открыть…");
    open_->setObjectName("openArchiveAction");
    QtBindLocalText(open_, "Open archive…", QStringLiteral("Открыть архив…"));
    open_->setShortcut(QKeySequence::Open);
    openInside_ = menu->addAction("Открыть внутри");
    openInside_->setObjectName("openInsideAction");
    openInside_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    openInsideOne_ = new QAction("Открыть внутри *", this);
    openInsideOne_->setObjectName("openInsideOneAction");
    openInsideParser_ = new QAction("Открыть внутри #", this);
    openInsideParser_->setObjectName("openInsideParserAction");
    openOutside_ = menu->addAction("Открыть снаружи");
    openOutside_->setObjectName("openOutsideAction");
    openOutside_->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return));
    create_ = menu->addAction("Создать архив…");
    create_->setObjectName("createArchiveAction");
    QtBindLocalText(create_, "Create archive…", QStringLiteral("Создать архив…"));
    add_ = menu->addAction("Добавить в открытый архив…");
    add_->setObjectName("addArchiveAction");
    QtBindLocalText(add_, "Add to open archive…", QStringLiteral("Добавить в открытый архив…"));
    toolbarAdd_ = toolbar->addAction("Добавить");
    toolbarAdd_->setObjectName("toolbarAddAction");
    extract_ = toolbar->addAction("Извлечь");
    extract_->setObjectName("extractArchiveAction");
    selected_ = menu->addAction("Извлечь выбранное…");
    selected_->setObjectName("extractSelectedAction");
    QtBindLocalText(selected_, "Extract selected…", QStringLiteral("Извлечь выбранное…"));
    journalAction_ = menu->addAction("Журнал последней операции…");
    journalAction_->setObjectName("operationJournalAction");
    QtBindLocalText(journalAction_, "Last operation log…",
                    QStringLiteral("Журнал последней операции…"));
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
    exitAction_ = new QAction("Выход", this);
    exitAction_->setObjectName("exitAction");
    exitAction_->setShortcut(QKeySequence::Quit);
}

void MainWindow::createFileExtras(QMenu *menu, QToolBar *toolbar) {
    auto *hashMenu = menu->addMenu("CRC");
    hashMenu->setObjectName("hashMenu");
    hashMenu->menuAction()->setObjectName("hashMenu");
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
    addFavoriteMenu->setObjectName("addFavoriteMenu");
    addFavoriteMenu->menuAction()->setObjectName("addFavoriteMenu");
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
    auto *timeMenu = viewMenu->addMenu(QStringLiteral("Время"));
    timeMenu->setObjectName(QStringLiteral("timestampMenu"));
    timeMenu->menuAction()->setObjectName("timestampMenu");
    auto *timeGroup = new QActionGroup(timeMenu);
    for (unsigned i = 0; i < 5; ++i) {
        QAction *action = timeMenu->addAction(QString());
        action->setObjectName(QStringLiteral("timestampLevel%1Action").arg(i));
        action->setCheckable(true);
        action->setActionGroup(timeGroup);
        timeActions_[i] = action;
    }
    utcTimeAction_ = timeMenu->addAction(QStringLiteral("UTC"));
    utcTimeAction_->setObjectName(QStringLiteral("timestampUtcAction"));
    utcTimeAction_->setCheckable(true);
    connect(timeMenu, &QMenu::aboutToShow, this, [this, timeMenu] {
        FILETIME now;
        NWindows::NTime::GetCurUtcFileTime(now);
        const int levels[] = {kTimestampPrintLevel_DAY, kTimestampPrintLevel_MIN,
                              kTimestampPrintLevel_SEC, kTimestampPrintLevel_NTFS,
                              kTimestampPrintLevel_NS};
        char caption[64];
        if (ConvertUtcFileTimeToString(now, caption, kTimestampPrintLevel_DAY))
            timeMenu->menuAction()->setText(QString::fromLatin1(caption));
        app_._timestampLevels.Clear();
        for (unsigned i = 0; i < 5; ++i) {
            if (ConvertUtcFileTimeToString(now, caption, levels[i])) {
                timeActions_[i]->setText(QString::fromLatin1(caption));
                app_._timestampLevels.Add(levels[i]);
            }
            timeActions_[i]->setChecked(app_.GetTimestampLevel() == levels[i]);
        }
        utcTimeAction_->setChecked(g_Timestamp_Show_UTC);
    });
    viewMenu->addSeparator();
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    fileListSettings_.Load(toolbarSettingsPath());
    flatDisk_ = settings.value("view/flatDisk", false).toBool();
    flatArchive_ = settings.value("view/flatArchive", false).toBool();
    flatView_ = viewMenu->addAction("Плоский вид");
    flatView_->setObjectName("flatViewAction");
    flatView_->setCheckable(true);
    flatView_->setChecked(flatDisk_);
    twoPanelsAction_ = viewMenu->addAction(QStringLiteral("Две панели"));
    twoPanelsAction_->setObjectName("twoPanelsAction");
    twoPanelsAction_->setCheckable(true);
    twoPanelsAction_->setChecked(app_.NumPanels == 2);
    twoPanelsAction_->setShortcut(QKeySequence(Qt::Key_F9));
    connect(twoPanelsAction_, &QAction::triggered, this, [this] { togglePanels(); });
    viewMenu->addSeparator();
    createToolbarOptions(viewMenu);
    root_ = viewMenu->addAction("Открыть корневую папку");
    root_->setObjectName("openRootAction");
    refresh_ = viewMenu->addAction("Обновить");
    refresh_->setObjectName("refreshAction");
    refresh_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    autoRefreshAction_ = viewMenu->addAction(QStringLiteral("Автоматическое обновление"));
    autoRefreshAction_->setObjectName(QStringLiteral("autoRefreshAction"));
    autoRefreshAction_->setCheckable(true);
    autoRefreshAction_->setChecked(app_.Get_AutoRefresh_Mode());
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
    toolbarsMenu->setObjectName("toolbarsMenu");
    toolbarsMenu->menuAction()->setObjectName("toolbarsMenu");
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

void MainWindow::rebuildSourceMenus() {
    // resource.rc IDM_MENU is the order/grouping contract. Keep existing Qt
    // actions so the source command dispatcher and toolbar share one action.
    auto reset = [](QMenu *menu) {
        for (QAction *action : menu->actions())
            menu->removeAction(action);
    };
    auto unavailable = [this](QMenu *menu, const QString &label, const char *name, unsigned id) {
        auto *action = new QAction(label, this);
        action->setObjectName(name);
        action->setProperty("nativeCommandID", id);
        action->setEnabled(false);
        menu->addAction(action);
    };

    auto *file = findChild<QMenu *>("fileMenu");
    auto *hash = findChild<QMenu *>("hashMenu");
    reset(file);
    for (QAction *action : {openItem_, openInside_, openInsideOne_, openInsideParser_, openOutside_,
                            viewFile_, editFile_})
        file->addAction(action);
    file->addSeparator();
    for (QAction *action : {rename_, copy_, move_, remove_})
        file->addAction(action);
    file->addSeparator();
    file->addAction(split_);
    file->addAction(combine_);
    file->addSeparator();
    file->addAction(properties_);
    unavailable(file, QStringLiteral("Комментарий…"), "commentAction", IDM_COMMENT);
    file->addMenu(hash);
    diffFiles_ = file->addAction(QStringLiteral("&Сравнить"));
    diffFiles_->setObjectName("diffAction");
    file->addSeparator();
    file->addAction(createFolder_);
    file->addAction(createFile_);
    file->addSeparator();
    unavailable(file, QStringLiteral("Ссылка…"), "linkAction", IDM_LINK);
    unavailable(file, QStringLiteral("Альтернативные потоки"), "altStreamsAction", IDM_ALT_STREAMS);
    file->addSeparator();
    // Existing cross-platform archive workflows are retained as a separate
    // Qt integration group until their source 7zFM entry points are ported.
    for (QAction *action : {open_, create_, add_, selected_, journalAction_})
        file->addAction(action);
    auto *textView = file->addAction("Просмотреть текст…");
    textView->setObjectName("builtinTextViewAction");
    QtBindLocalText(textView, "View text…", QStringLiteral("Просмотреть текст…"));
    auto *textEdit = file->addAction("Редактировать текст…");
    textEdit->setObjectName("builtinTextEditAction");
    QtBindLocalText(textEdit, "Edit text…", QStringLiteral("Редактировать текст…"));
    file->addSeparator();
    file->addAction(exitAction_);
    const char *verCtrlNames[] = {"verEditAction", "verCommitAction", "verRevertAction",
                                  "verDiffAction"};
    for (unsigned i = 0; i < 4; ++i) {
        auto *action = file->addAction(QString());
        action->setObjectName(QString::fromLatin1(verCtrlNames[i]));
        action->setProperty("nativeCommandID", IDM_VER_EDIT + i);
        action->setVisible(false);
    }

    auto *edit = findChild<QMenu *>("editMenu");
    reset(edit);
    for (QAction *action : {selectAll_, deselectAll_, invertSelection_, selectMask_, deselectMask_})
        edit->addAction(action);
    edit->addSeparator();
    edit->addAction(selectType_);
    edit->addAction(deselectType_);

    auto *view = findChild<QMenu *>("viewMenu");
    auto *time = findChild<QMenu *>("timestampMenu");
    auto *toolbars = findChild<QMenu *>("toolbarsMenu");
    reset(view);
    for (QAction *action : viewActions_)
        view->addAction(action);
    view->addSeparator();
    for (QAction *action : sortActions_)
        view->addAction(action);
    view->addSeparator();
    view->addAction(flatView_);
    view->addAction(twoPanelsAction_);
    view->addMenu(time);
    view->addMenu(toolbars);
    for (QAction *action : {root_, up_, foldersHistory_, refresh_, autoRefreshAction_})
        view->addAction(action);

    auto *favorites = findChild<QMenu *>("favoritesMenu");
    auto *addFavorite = findChild<QMenu *>("addFavoriteMenu");
    reset(favorites);
    favorites->addMenu(addFavorite);
    favorites->addSeparator();
    for (QAction *action : openBookmarks_)
        favorites->addAction(action);

    auto *tools = findChild<QMenu *>("toolsMenu");
    reset(tools);
    tools->addAction(settingsAction_);
    tools->addSeparator();
    unavailable(tools, QStringLiteral("Тест производительности"), "benchmarkAction", IDM_BENCHMARK);
    tools->addSeparator();
    unavailable(tools, QStringLiteral("Удалить временные файлы…"), "tempDirAction", IDM_TEMP_DIR);

    auto *help = findChild<QMenu *>("helpMenu");
    unavailable(help, QStringLiteral("Содержание…"), "helpContentsAction", IDM_HELP_CONTENTS);
    help->addSeparator();
    unavailable(help, QStringLiteral("О 7-Zip…"), "aboutAction", IDM_ABOUT);
}
