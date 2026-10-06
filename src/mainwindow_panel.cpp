#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"
#include "mainwindow.h"
#include "qtfilelisthover.h"
#include "qtfilemanagerdrag.h"
#include "qtlangcontrols.h"
#include "qtpaneladdresscombo.h"
#include "qtstatuslabel.h"

void MainWindow::createFilePanel(QMenu *editMenu, unsigned index) {
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);
    auto *navigation = new QHBoxLayout;
    navigation->setContentsMargins(0, 0, 0, 0);
    for (QAction *action : {back_, up_}) {
        auto *button = new QToolButton(central);
        button->setDefaultAction(action);
        // Panel.cpp's parent toolbar uses the standard VIEW_PARENTFOLDER image,
        // without its longer View menu caption. Qt supplies the platform arrow.
        const auto updateIcon = [this, button, action] {
            button->setIcon(
                style()->standardIcon(action == up_ ? QStyle::SP_ArrowUp : QStyle::SP_ArrowBack));
        };
        updateIcon();
        // QAction::changed also projects its empty menu icon to QToolButton.
        // Restore the panel-specific image after every caption/state update.
        connect(action, &QAction::changed, button, updateIcon);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setObjectName(
            QString(action == up_ ? "panelParentButton%1" : "panelBackButton%1").arg(index));
        navigation->addWidget(button);
    }
    pathCombo_ = new QtPanelAddressCombo(
        app_.Panels[index],
        [this, index](const QString &path, bool sourcePath) {
            if (app_.QtOperations().closed())
                return E_ABORT;
            activatePanelUi(index);
            return navigateTo(path, sourcePath);
        },
        central);
    pathCombo_->setObjectName("pathCombo");
    pathCombo_->setEditable(true);
    pathCombo_->setInsertPolicy(QComboBox::NoInsert);
    pathCombo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    pathCombo_->setMinimumContentsLength(16);
    path_ = pathCombo_->lineEdit();
    path_->setObjectName("archivePath");
    path_->installEventFilter(this);
    pathCombo_->installEventFilter(this);
    path_->setProperty("languagePlaceholder", true);
    QtBindLocalText(path_, "Path to folder or archive", "Путь к папке или архиву", true);
    navigation->addWidget(pathCombo_);
    layout->addLayout(navigation);
    createFileTable(central);
    if (!panelSplitter_)
        createPanelActions(editMenu);
    createIconList(central);
    viewStack_ = new QStackedWidget(central);
    viewStack_->setObjectName("fileViewStack");
    viewStack_->addWidget(table_);
    viewStack_->addWidget(iconList_);
    layout->addWidget(viewStack_);
    auto *panelStatus = new QStatusBar(central);
    panelStatus->setObjectName("panelStatusBar");
    panelStatus->setSizeGripEnabled(false);
    const char *statusNames[] = {"selectedItemsStatus", "selectedSizeStatus", "focusedSizeStatus",
                                 "focusedDateStatus"};
    for (unsigned part = 0; part < 4; ++part) {
        auto *label = new QtStatusLabel(panelStatus);
        label->setObjectName(statusNames[part]);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        panelStatus->addPermanentWidget(label, part == 0 ? 1 : 0);
        panelUi_[index].statusLabels[part] = label;
    }
    layout->addWidget(panelStatus);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this, index] {
        if (projectedPanel_ == index && app_.Panels[index].ShouldProcessNotifications())
            refreshActions();
    });
    connect(table_, &QTableWidget::currentCellChanged, this, [this, index] {
        if (projectedPanel_ == index && app_.Panels[index].ShouldProcessNotifications())
            refreshActions();
    });
    setViewMode(static_cast<int>(app_.GetFocusedPanel().GetListViewMode()));
    if (!panelSplitter_) {
        panelSplitter_ = new QSplitter(Qt::Horizontal, this);
        panelSplitter_->setObjectName("filePanelSplitter");
        panelSplitter_->setChildrenCollapsible(false);
        setCentralWidget(panelSplitter_);
    }
    panelUi_[index].container = central;
    central->setObjectName(QStringLiteral("filePanel%1").arg(index));
    panelSplitter_->addWidget(central);
    statusBar()->showMessage("Готово");
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
    QtBindLocalText(table_, "File list", "Список файлов", true);
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
    const unsigned columnOwner = projectedPanel_;
    connect(table_->horizontalHeader(), &QWidget::customContextMenuRequested, this,
            [this, columnOwner](const QPoint &pos) {
                withPanelProjection(columnOwner, [this, pos] { showColumnsMenu(pos); });
            });
    connect(table_->horizontalHeader(), &QHeaderView::sectionResized, this,
            [this, columnOwner] { withPanelProjection(columnOwner, [this] { saveColumns(); }); });
    connect(table_->horizontalHeader(), &QHeaderView::sectionMoved, this,
            [this, columnOwner] { withPanelProjection(columnOwner, [this] { saveColumns(); }); });
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableHover_ = new QtFileListHover(*table_);
    tableHover_->SetEnabled(fileListSettings_.SingleClick);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(table_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (item != renameEditingItem_ ||
            (state_ != State::Filesystem && state_ != State::Viewing &&
             !app_.GetFocusedPanel().IsAltStreamsFolder()))
            return;
        const QString name = item->data(Qt::EditRole).toString();
        const QString path = renameOriginalPath_;
        const quint32 sourceIndex = renameSourceIndex_;
        renameEditingItem_ = nullptr;
        renameOriginalPath_.clear();
        if (name ==
            (state_ == State::Viewing ? path.section('/', -1) : QFileInfo(path).fileName())) {
            if (state_ == State::Viewing && flatView_->isChecked())
                QTimer::singleShot(0, this, [this, path] {
                    renderBoundFolder();
                    selectPaths({path});
                });
            return;
        }
        QTimer::singleShot(0, this, [this, sourceIndex, path, name] {
            finishFilesystemRename(sourceIndex, path, name);
            renameActivationPending_ = false;
        });
    });
    connect(table_->itemDelegate(), &QAbstractItemDelegate::closeEditor, this,
            [this](QWidget *, QAbstractItemDelegate::EndEditHint) {
                renameEditingItem_ = nullptr;
                renameOriginalPath_.clear();
                QTimer::singleShot(0, this, [this] { renameActivationPending_ = false; });
            });
    table_->setSortingEnabled(true);
    table_->setDragEnabled(true);
    table_->setAcceptDrops(true);
    table_->viewport()->setAcceptDrops(true);
    table_->sortItems(0, Qt::AscendingOrder);
    connect(table_->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this,
            [this, owner = projectedPanel_](int column, Qt::SortOrder order) {
                sortIndicatorChanged(owner, column, order);
            });
    table_->installEventFilter(this);
    table_->viewport()->installEventFilter(this);
    connect(table_, &QTableWidget::cellClicked, this,
            [this](int row, int) { handleSelectionClick(row); });
    table_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table_, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        const bool keyboard = pos.x() < 0 && pos.y() < 0;
        const int row = keyboard ? table_->currentRow() : table_->rowAt(pos.y());
        if (row >= 0 && !table_->selectionModel()->isRowSelected(row, QModelIndex()))
            table_->selectRow(row);
        const QPoint anchor = keyboard && row >= 0
                                  ? table_->visualItemRect(table_->item(row, 0)).center()
                                  : (keyboard ? QPoint(0, 0) : pos);
        showItemContextMenu(table_->viewport()->mapToGlobal(anchor));
    });
    app_.GetFocusedPanel().AttachSelectionView(fileTable, [fileTable](PROPID property, Int32) {
        return fileTable->columnForProperty(property);
    });
}

void MainWindow::createPanelActions(QMenu *editMenu) {
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
    rebuildSourceMenus();
}

void MainWindow::createIconList(QWidget *central) {
    iconList_ = new FileIconList(central);
    iconList_->setObjectName("iconList");
    QtBindLocalText(iconList_, "File list", "Список файлов", true);
    iconList_->setModel(table_->model());
    iconList_->setItemDelegate(CreateFileRowDelegate(iconList_));
    iconList_->setModelColumn(0);
    iconList_->setSelectionModel(table_->selectionModel());
    iconList_->setSelectionMode(fileListSettings_.AlternativeSelection
                                    ? QAbstractItemView::SingleSelection
                                    : QAbstractItemView::ExtendedSelection);
    iconList_->setSelectionBehavior(QAbstractItemView::SelectRows);
    iconHover_ = new QtFileListHover(*iconList_);
    iconHover_->SetEnabled(fileListSettings_.SingleClick);
    iconList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(iconList_->itemDelegate(), &QAbstractItemDelegate::closeEditor, this,
            [this](QWidget *, QAbstractItemDelegate::EndEditHint) {
                renameEditingItem_ = nullptr;
                renameOriginalPath_.clear();
                QTimer::singleShot(0, this, [this] { renameActivationPending_ = false; });
            });
    iconList_->setUniformItemSizes(true);
    iconList_->setWordWrap(false);
    iconList_->setDragEnabled(true);
    iconList_->setAcceptDrops(true);
    iconList_->viewport()->setAcceptDrops(true);
    iconList_->setContextMenuPolicy(Qt::CustomContextMenu);
    iconList_->installEventFilter(this);
    iconList_->viewport()->installEventFilter(this);
    connect(iconList_, &QListView::clicked, this,
            [this](const QModelIndex &index) { handleSelectionClick(index.row()); });
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
        const bool keyboard = pos.x() < 0 && pos.y() < 0;
        const QModelIndex index = keyboard ? iconList_->currentIndex() : iconList_->indexAt(pos);
        if (index.isValid() &&
            !iconList_->selectionModel()->isRowSelected(index.row(), QModelIndex()))
            iconList_->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect |
                                                           QItemSelectionModel::Rows);
        const QPoint anchor = keyboard && index.isValid() ? iconList_->visualRect(index).center()
                                                          : (keyboard ? QPoint(0, 0) : pos);
        showItemContextMenu(iconList_->viewport()->mapToGlobal(anchor));
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
            item->setToolTip(QtLocalText("Parent folder", "Родительская папка"));
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
    if (drag_ && drag_->Active())
        return;
    // Selection modifiers must keep the Qt multi-selection gesture intact.
    const auto modifiers = QApplication::keyboardModifiers();
    if (fileListSettings_.SingleClick &&
        !(modifiers & (Qt::ControlModifier | Qt::MetaModifier | Qt::ShiftModifier)))
        activateRow(row);
}
