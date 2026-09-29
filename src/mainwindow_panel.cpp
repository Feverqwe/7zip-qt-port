#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "mainwindow.h"
#include "qtfilelisthover.h"

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
    refreshPathHistory();
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
    setViewMode(static_cast<int>(app_.GetFocusedPanel().GetListViewMode()));
    setCentralWidget(central);
    statusBar()->showMessage("Готово");
    connect(path_, &QLineEdit::returnPressed, this, [this] { navigateTo(path_->text()); });
    connect(pathCombo_, &QComboBox::activated, this,
            [this](int index) { navigateTo(pathCombo_->itemText(index)); });
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (!fileListSettings_.SingleClick)
            activateRow(row);
    });
    connect(table_, &QTableWidget::cellClicked, this,
            [this](int row, int) { activateSingleClickedRow(row); });
}

void MainWindow::createFileTable(QWidget *central) {
    table_ = new FileTable(0, 7, central);
    auto *fileTable = static_cast<FileTable *>(table_);
    fileTable->setFullRowHighlight(fileListSettings_.FullRow);
    fileTable->sourcePanel = &app_.GetFocusedPanel();
    fileTable->propertyIds = {kpidName,  kpidPath,      kpidSize,      kpidPackSize,
                              kpidMTime, kpidExtension, kpidNoProperty};
    fileTable->rawProperties.resize(7);
    table_->setObjectName("archiveTable");
    table_->setAccessibleName(QStringLiteral("Список файлов"));
    table_->verticalHeader()->hide();
    table_->setHorizontalHeaderLabels(
        {"Имя", "Путь", "Размер", "Сжатый", "Изменён", "Тип", "Порядок"});
    for (int column : {2, 3, 4})
        table_->horizontalHeaderItem(column)->setData(Qt::InitialSortOrderRole,
                                                      Qt::DescendingOrder);
    table_->setColumnHidden(1, true);
    table_->setColumnHidden(5, true);
    table_->setColumnHidden(6, true);
    table_->horizontalHeader()->setSectionsMovable(true);
    table_->setColumnWidth(0, 160);
    table_->setColumnWidth(2, 100);
    table_->setColumnWidth(3, 100);
    table_->setColumnWidth(4, 150);
    table_->setShowGrid(fileListSettings_.ShowGrid);
    table_->setWordWrap(false);
    table_->setTextElideMode(Qt::ElideMiddle);
    table_->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table_->horizontalHeader(), &QWidget::customContextMenuRequested, this,
            [this](const QPoint &pos) { showColumnsMenu(pos); });
    connect(table_->horizontalHeader(), &QHeaderView::sectionResized, this,
            [this] { saveColumns(); });
    connect(table_->horizontalHeader(), &QHeaderView::sectionMoved, this,
            [this] { saveColumns(); });
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableHover_ = new QtFileListHover(*table_);
    tableHover_->SetEnabled(fileListSettings_.SingleClick);
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
                auto &panel = app_.GetFocusedPanel();
                panel._sortID = static_cast<FileTable *>(table_)->propertyId(column);
                panel._ascending = order == Qt::AscendingOrder;
                if (!configuringColumns_) {
                    const int mode = column == 5   ? 1
                                     : column == 4 ? 2
                                     : column == 2 ? 3
                                     : column == 6 ? 4
                                                   : 0;
                    sortMode_ = static_cast<SortMode>(mode);
                    if (column == 0 || column == 2 || column == 4 || column == 5 || column == 6)
                        sortActions_[mode]->setChecked(true);
                    saveColumns();
                }
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

void MainWindow::configureColumns() {
    auto &panel = app_.GetFocusedPanel();
    if (panel.InitColumns() != S_OK)
        return;
    configuringColumns_ = true;
    const QSignalBlocker blocked(table_->horizontalHeader());
    auto *fileTable = static_cast<FileTable *>(table_);
    const CPropColumns &columns = panel.GetColumns();
    int extraCount = 0;
    FOR_VECTOR(i, columns) {
        const PROPID id = columns[i].ID;
        if (id != kpidName && id != kpidSize && id != kpidPackSize && id != kpidMTime)
            ++extraCount;
    }
    table_->setRowCount(0);
    table_->setColumnCount(7 + extraCount);
    fileTable->propertyIds = {kpidName,  kpidPath,      kpidSize,      kpidPackSize,
                              kpidMTime, kpidExtension, kpidNoProperty};
    fileTable->rawProperties.fill(false, 7 + extraCount);
    QStringList labels = {"Имя", "Путь", "Размер", "Сжатый", "Изменён", "Тип", "Порядок"};
    int next = 7;
    FOR_VECTOR(i, columns) {
        const CPropColumn &prop = columns[i];
        int column = -1;
        if (prop.ID == kpidName)
            column = 0;
        else if (prop.ID == kpidSize)
            column = 2;
        else if (prop.ID == kpidPackSize)
            column = 3;
        else if (prop.ID == kpidMTime)
            column = 4;
        else {
            column = next++;
            fileTable->propertyIds.append(prop.ID);
            labels << ToQString(prop.Name);
        }
        fileTable->rawProperties[column] = prop.IsRawProp;
        labels[column] = ToQString(prop.Name);
        table_->setColumnWidth(column, qMax(32, int(prop.Width)));
        table_->setColumnHidden(column, !prop.IsVisible);
    }
    table_->setHorizontalHeaderLabels(labels);
    for (int column : {2, 3, 4})
        table_->horizontalHeaderItem(column)->setData(Qt::InitialSortOrderRole,
                                                      Qt::DescendingOrder);
    for (int column : {1, 5, 6})
        table_->setColumnHidden(column, true);
    for (int column : {2, 3, 4}) {
        if (columns.FindItem_for_PropID(fileTable->propertyId(column)) < 0)
            table_->setColumnHidden(column, true);
    }
    const CPropColumns &visible = panel.GetVisibleColumns();
    FOR_VECTOR(i, visible) {
        const int column = fileTable->columnForProperty(visible[i].ID);
        if (column >= 0)
            table_->horizontalHeader()->moveSection(table_->horizontalHeader()->visualIndex(column),
                                                    int(i));
    }
    sortColumn_ = fileTable->columnForProperty(panel._sortID);
    if (sortColumn_ < 0 || table_->isColumnHidden(sortColumn_))
        sortColumn_ = panel._sortID == kpidNoProperty ? 6 : 0;
    sortOrder_ = panel._ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
    configuringColumns_ = false;
}

void MainWindow::saveColumns() {
    if (configuringColumns_ || !table_)
        return;
    auto &panel = app_.GetFocusedPanel();
    if (panel.GetColumns().IsEmpty())
        return;
    const auto *fileTable = static_cast<FileTable *>(table_);
    CPropColumns visible;
    const CPropColumns &columns = panel.GetColumns();
    FOR_VECTOR(i, columns) {
        const int column = fileTable->columnForProperty(columns[i].ID);
        if (column < 0 || table_->isColumnHidden(column))
            continue;
        CPropColumn prop = columns[i];
        prop.IsVisible = true;
        prop.Order = table_->horizontalHeader()->visualIndex(column);
        prop.Width = UInt32(table_->horizontalHeader()->sectionSize(column));
        visible.Add(prop);
    }
    panel.SaveListViewInfo(visible);
}

void MainWindow::showColumnsMenu(const QPoint &pos) {
    auto &panel = app_.GetFocusedPanel();
    const CPropColumns &columns = panel.GetColumns();
    auto *fileTable = static_cast<FileTable *>(table_);
    QMenu menu(this);
    FOR_VECTOR(i, columns) {
        const CPropColumn &prop = columns[i];
        const int column = fileTable->columnForProperty(prop.ID);
        if (column < 0)
            continue;
        QAction *option = menu.addAction(ToQString(prop.Name));
        option->setCheckable(true);
        option->setChecked(!table_->isColumnHidden(column));
        option->setEnabled(prop.ID != kpidName);
        connect(option, &QAction::toggled, this, [this, column](bool visible) {
            table_->setColumnHidden(column, !visible);
            if (visible) {
                const bool sorting = table_->isSortingEnabled();
                table_->setSortingEnabled(false);
                for (int row = 0; row < table_->rowCount(); ++row) {
                    QTableWidgetItem *item = table_->item(row, column);
                    if (item)
                        item->setText(
                            sourceColumnText(item->data(Qt::UserRole + 6).toUInt(), column));
                }
                table_->setSortingEnabled(sorting);
            }
            if (!visible && sortColumn_ == column) {
                sortColumn_ = 0;
                sortOrder_ = Qt::AscendingOrder;
                applySortMode();
            }
            saveColumns();
        });
    }
    menu.exec(table_->horizontalHeader()->mapToGlobal(pos));
}

QString MainWindow::sourceColumnText(UInt32 itemIndex, int column) {
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const auto &panel = app_.GetFocusedPanel();
    const CPropColumns &columns = panel.GetColumns();
    const int index = columns.FindItem_for_PropID(fileTable->propertyId(column));
    return index >= 0 ? panel.GetItemText(itemIndex, columns[index]) : QString();
}

Qt::Alignment MainWindow::sourceColumnAlignment(int column) const {
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const CPanel &panel = app_.Panels[app_.GetFocusedPanelIndex()];
    const CPropColumns &columns = panel.GetColumns();
    const int index = columns.FindItem_for_PropID(fileTable->propertyId(column));
    return index >= 0 ? GetColumnAlign(columns[index].ID, columns[index].Type)
                      : Qt::AlignLeft | Qt::AlignVCenter;
}

void MainWindow::redrawTimestampColumns() {
    if (!table_ || !app_.GetFocusedPanel().GetFolder())
        return;
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const CPropColumns &columns = app_.GetFocusedPanel().GetColumns();
    const bool sorting = table_->isSortingEnabled();
    table_->setSortingEnabled(false);
    for (int column = 0; column < table_->columnCount(); ++column) {
        const int index = columns.FindItem_for_PropID(fileTable->propertyId(column));
        if (index < 0 || columns[index].Type != VT_FILETIME || table_->isColumnHidden(column))
            continue;
        for (int row = 0; row < table_->rowCount(); ++row) {
            QTableWidgetItem *item = table_->item(row, column);
            QTableWidgetItem *name = table_->item(row, 0);
            if (item && name)
                item->setText(sourceColumnText(name->data(Qt::UserRole + 6).toUInt(), column));
        }
    }
    table_->setSortingEnabled(sorting);
}

void MainWindow::createPanelActions(QMenu *editMenu) {
    openItem_ = new QAction(table_);
    openItem_->setObjectName("openItemAction");
    table_->addAction(openItem_);
    parent_ = new QAction(table_);
    parent_->setObjectName("parentFolderAction");
    table_->addAction(parent_);
    selectAll_ = new QAction(table_);
    selectAll_->setObjectName("selectAllAction");
    selectAll_->setShortcut(QKeySequence::SelectAll);
    selectAll_->setShortcutContext(Qt::WidgetShortcut);
    table_->addAction(selectAll_);
    selectAll_->setText("Выделить всё");
    editMenu->addAction(selectAll_);
    deselectAll_ = editMenu->addAction("Снять выделение");
    deselectAll_->setObjectName("deselectAllAction");
    invertSelection_ = editMenu->addAction("Инвертировать выделение");
    invertSelection_->setObjectName("invertSelectionAction");
    selectMask_ = editMenu->addAction("Выделить по маске…");
    selectMask_->setObjectName("selectMaskAction");
    deselectMask_ = editMenu->addAction("Снять выделение по маске…");
    deselectMask_->setObjectName("deselectMaskAction");
    selectType_ = editMenu->addAction("Выделить по типу");
    selectType_->setObjectName("selectTypeAction");
    deselectType_ = editMenu->addAction("Снять выделение по типу");
    deselectType_->setObjectName("deselectTypeAction");
}

void MainWindow::createIconList(QWidget *central) {
    iconList_ = new QListView(central);
    iconList_->setObjectName("iconList");
    iconList_->setAccessibleName(QStringLiteral("Список файлов"));
    iconList_->setModel(table_->model());
    iconList_->setModelColumn(0);
    iconList_->setSelectionModel(table_->selectionModel());
    iconList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    iconList_->setSelectionBehavior(QAbstractItemView::SelectRows);
    iconHover_ = new QtFileListHover(*iconList_);
    iconHover_->SetEnabled(fileListSettings_.SingleClick);
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
        if (!fileListSettings_.SingleClick)
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

void MainWindow::appendParentEntry() {
    if (!app_.GetFocusedPanel()._showDots || app_.GetFocusedPanel().IsRootFolder())
        return;
    const bool sorting = table_->isSortingEnabled();
    table_->setSortingEnabled(false);
    const int row = table_->rowCount();
    table_->insertRow(row);
    for (int column = 0; column < table_->columnCount(); ++column) {
        auto *item = new ArchiveTableItem(column == 0 ? QStringLiteral("..") : QString());
        item->setData(Qt::UserRole + 1, true);
        item->setData(Qt::UserRole + 3, QStringLiteral(".."));
        item->setData(Qt::UserRole + 6, quint32(-1));
        if (column == 0) {
            item->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
            item->setToolTip(QStringLiteral("Родительская папка"));
        }
        table_->setItem(row, column, item);
    }
    table_->setSortingEnabled(sorting);
}

void MainWindow::updateParentEntry() {
    if (!table_)
        return;
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (table_->item(row, 0) &&
            table_->item(row, 0)->data(Qt::UserRole + 6).toUInt() == UInt32(-1)) {
            table_->removeRow(row);
            break;
        }
    }
    if (state_ != State::Namespace && state_ != State::Loading && state_ != State::Operating)
        appendParentEntry();
    applySortMode();
    refreshActions();
}

void MainWindow::activateSingleClickedRow(int row) {
    // Selection modifiers must keep the Qt multi-selection gesture intact.
    const auto modifiers = QApplication::keyboardModifiers();
    if (fileListSettings_.SingleClick &&
        !(modifiers & (Qt::ControlModifier | Qt::MetaModifier | Qt::ShiftModifier)))
        activateRow(row);
}
