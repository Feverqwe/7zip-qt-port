#include "mainwindow.h"

void MainWindow::createFilePanel(QMenu *editMenu) {
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
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
    pathCombo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    pathCombo_->setMinimumContentsLength(16);
    path_ = pathCombo_->lineEdit();
    path_->setObjectName("archivePath");
    path_->installEventFilter(this);
    pathCombo_->installEventFilter(this);
    path_->setPlaceholderText("Путь к папке или архиву");
    for (const QString &visited : settings.value("history/paths").toStringList()) {
        if (pathCombo_->count() >= 20)
            break;
        pathCombo_->addItem(visited);
    }
    navigation->addWidget(pathCombo_);
    layout->addLayout(navigation);
    createFileTable(central);
    createPanelActions(editMenu);
    createIconList(central);
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
    connect(table_, &QTableWidget::cellClicked, this,
            [this](int row, int) { activateSingleClickedRow(row); });
}

void MainWindow::createFileTable(QWidget *central) {
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    table_ = new FileTable(0, 7, central);
    table_->setObjectName("archiveTable");
    table_->verticalHeader()->hide();
    table_->setHorizontalHeaderLabels(
        {"Имя", "Путь", "Размер", "Сжатый", "Изменён", "Тип", "Порядок"});
    for (int column : {2, 3, 4})
        table_->horizontalHeaderItem(column)->setData(Qt::InitialSortOrderRole,
                                                      Qt::DescendingOrder);
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
            [this](int column, Qt::SortOrder order) {
                sortColumn_ = column;
                sortOrder_ = order;
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
}

void MainWindow::createPanelActions(QMenu *editMenu) {
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
    connect(selectAll_, &QAction::triggered, this, [this] { SelectAll(true); });
    selectAll_->setText("Выделить всё");
    editMenu->addAction(selectAll_);
    deselectAll_ = editMenu->addAction("Снять выделение", this, [this] { SelectAll(false); });
    deselectAll_->setObjectName("deselectAllAction");
    invertSelection_ =
        editMenu->addAction("Инвертировать выделение", this, [this] { InvertSelection(); });
    invertSelection_->setObjectName("invertSelectionAction");
    selectMask_ = editMenu->addAction("Выделить по маске…", this, [this] { SelectSpec(true); });
    selectMask_->setObjectName("selectMaskAction");
    deselectMask_ =
        editMenu->addAction("Снять выделение по маске…", this, [this] { SelectSpec(false); });
    deselectMask_->setObjectName("deselectMaskAction");
    selectType_ = editMenu->addAction("Выделить по типу", this, [this] { SelectByType(true); });
    selectType_->setObjectName("selectTypeAction");
    deselectType_ =
        editMenu->addAction("Снять выделение по типу", this, [this] { SelectByType(false); });
    deselectType_->setObjectName("deselectTypeAction");
}

void MainWindow::createIconList(QWidget *central) {
    iconList_ = new QListView(central);
    iconList_->setObjectName("iconList");
    iconList_->setModel(table_->model());
    iconList_->setModelColumn(0);
    iconList_->setSelectionModel(table_->selectionModel());
    iconList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    iconList_->setSelectionBehavior(QAbstractItemView::SelectRows);
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
    connect(iconList_, &QListView::clicked, this,
            [this](const QModelIndex &index) { activateSingleClickedRow(index.row()); });
    connect(iconList_, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex index = iconList_->indexAt(pos);
        if (index.isValid() &&
            !iconList_->selectionModel()->isRowSelected(index.row(), QModelIndex()))
            iconList_->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect |
                                                           QItemSelectionModel::Rows);
        showItemContextMenu(iconList_->viewport()->mapToGlobal(pos));
    });
}

void MainWindow::activateSingleClickedRow(int row) {
    // Selection modifiers must keep the Qt multi-selection gesture intact.
    const auto modifiers = QApplication::keyboardModifiers();
    if (fileListSettings_.singleClick &&
        !(modifiers & (Qt::ControlModifier | Qt::MetaModifier | Qt::ShiftModifier)))
        activateRow(row);
}
