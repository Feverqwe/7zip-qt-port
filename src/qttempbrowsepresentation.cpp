// Qt controls for the mirrored upstream BrowseDialog2 workflow/resource.
#include "7zip/UI/FileManager/BrowseDialog2Res.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog2.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariantConv.h"
#include "Windows/TimeUtils.h"
#include "qtfilelisthover.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QStyledItemDelegate>
#include <QTreeWidget>
#include <QVBoxLayout>

wchar_t *Browse_ConvertSizeToString(UInt64 v, wchar_t *s);
namespace {
QString Text(const UString &s) { return QString::fromWCharArray(s.Ptr()); }
class BrowseRow final : public QTreeWidgetItem {
    CBrowseDialog2 &_dialog;
    bool operator<(const QTreeWidgetItem &other) const override {
        return _dialog.CompareItems(data(0, Qt::UserRole).toInt(),
                                    other.data(0, Qt::UserRole).toInt()) < 0;
    }

  public:
    BrowseRow(CBrowseDialog2 &dialog, QTreeWidget *list) : QTreeWidgetItem(list), _dialog(dialog) {}
};
class BrowseAppearance final : public QStyledItemDelegate {
    bool _fullRow, _grid;

  public:
    BrowseAppearance(bool fullRow, bool grid, QObject *parent)
        : QStyledItemDelegate(parent), _fullRow(fullRow), _grid(grid) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        auto display = option;
        if (!_fullRow && index.column() != 0)
            display.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
        QStyledItemDelegate::paint(painter, display, index);
        if (_grid) {
            painter->save();
            painter->setPen(option.palette.color(QPalette::Mid));
            painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
            painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
            painter->restore();
        }
    }
};
} // namespace
CBrowseDialog2::CBrowseDialog2(QWidget *parent) : QDialog(parent) { BuildControls(); }
void CBrowseDialog2::BuildControls() {
    setObjectName("tempBrowseDialog");
    resize(900, 580);
    setMinimumSize(500, 320);
    auto *layout = new QVBoxLayout(this);
    auto *commands = new QHBoxLayout;
    _delete = new QPushButton(this);
    _delete->setObjectName("tempDelete");
    _delete->setProperty("sourceControlID", IDS_BUTTON_DELETE);
    QtBindResourceText(_delete, IDS_BUTTON_DELETE, "Delete");
    auto *refresh = new QPushButton(this);
    refresh->setObjectName("tempRefresh");
    refresh->setProperty("sourceControlID", IDM_VIEW_REFRESH);
    QtBindResourceText(refresh, IDM_VIEW_REFRESH, "Refresh");
    commands->addWidget(_delete);
    commands->addWidget(refresh);
    commands->addStretch();
    layout->addLayout(commands);
    auto *path = new QHBoxLayout;
    _parent = new QPushButton("<--", this);
    _parent->setObjectName("tempParent");
    _parent->setProperty("sourceControlID", IDB_BROWSE2_PARENT);
    QtBindLocalText(_parent, "Parent folder", "Родительская папка", true);
    path->addWidget(_parent);
    _pathEdit = new QLineEdit(this);
    _pathEdit->setReadOnly(true);
    _pathEdit->setObjectName("tempPath");
    _pathEdit->setProperty("sourceControlID", IDT_BROWSE2_FOLDER);
    QtBindLocalText(_pathEdit, "Temporary folder", "Временная папка", true);
    path->addWidget(_pathEdit, 1);
    layout->addLayout(path);
    _list = new QTreeWidget(this);
    _list->setObjectName("tempList");
    _list->setProperty("sourceControlID", IDL_BROWSE2);
    _list->setRootIsDecorated(false);
    _list->setUniformRowHeights(true);
    _list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // Source OnInit loads the same FM preferences before constructing the list.
    CFmSettings st;
    st.Load(FmSettingsPath());
    _showDots = st.ShowDots;
    _singleClick = st.SingleClick;
    _list->setItemDelegate(new BrowseAppearance(st.FullRow, st.ShowGrid, _list));
    auto *hover = new QtFileListHover(*_list);
    hover->SetEnabled(st.SingleClick);
    _list->setColumnCount(6);
    _list->header()->setStretchLastSection(false);
    _list->header()->setSectionsClickable(true);
    for (int i = 0; i < 6; ++i)
        _list->setColumnWidth(i, i == 1 ? 165 : 130);
    _list->installEventFilter(this);
    _list->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(_list, 1);
    _filterCombo = new QComboBox(this);
    _filterCombo->setObjectName("tempFilter");
    _filterCombo->setProperty("sourceControlID", IDC_BROWSE2_FILTER);
    _filterCombo->addItem(
        QtLocalText("Sevenzip Desktop owned temporary files", "Временные файлы Sevenzip Desktop"));
    _filterCombo->setEnabled(false);
    layout->addWidget(_filterCombo);
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Close, this);
    buttons->setObjectName("tempFooter");
    buttons->button(QDialogButtonBox::Help)->setObjectName("tempHelp");
    buttons->button(QDialogButtonBox::Close)->setObjectName("tempClose");
    layout->addWidget(buttons);
    ConnectControls(buttons, refresh);
}
void CBrowseDialog2::ConnectControls(QDialogButtonBox *buttons, QPushButton *refresh) {
    for (auto *button : findChildren<QPushButton *>()) {
        button->setAutoDefault(false);
        button->setDefault(false);
    }
    connect(_delete, &QPushButton::clicked, this, [this] {
        OnDelete();
        _list->setFocus();
    });
    connect(refresh, &QPushButton::clicked, this, [this] {
        Reload_WithErrorMessage();
        _list->setFocus();
    });
    connect(_parent, &QPushButton::clicked, this, [this] {
        OpenParentFolder();
        _list->setFocus();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::helpRequested, this, &CBrowseDialog2::OnHelp);
    connect(_list, &QTreeWidget::itemActivated, this, [this] {
        if (!_singleClick)
            OnItemEnter();
    });
    connect(_list, &QTreeWidget::itemClicked, this, [this] {
        if (_singleClick && QApplication::keyboardModifiers() == Qt::NoModifier)
            OnItemEnter();
    });
    connect(_list, &QTreeWidget::customContextMenuRequested, this, &CBrowseDialog2::OnContextMenu);
    connect(_list, &QTreeWidget::itemSelectionChanged, this,
            &CBrowseDialog2::UpdateDeleteAvailability);
    connect(_list->header(), &QHeaderView::sectionClicked, this,
            &CBrowseDialog2::ChangeSorting_and_Reload);
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, &CBrowseDialog2::OnHelp);
    _list->setFocus();
}
void CBrowseDialog2::PopulateRows(const UStringVector &selected, const UString &focused) {
    _list->setUpdatesEnabled(false);
    _list->clear();
    _parent->setEnabled(!IsExactTempFolder(DirPrefix));
    _pathEdit->setText(Text(DirPrefix));
    _list->setHeaderLabels({QtLangText(IDS_PROP_NAME), QtLangText(IDS_PROP_MTIME),
                            QtLangText(IDS_PROP_SIZE), QtLangText(IDS_PROP_FILES),
                            QtLangText(IDS_PROP_FOLDERS), QtLangText(IDS_PROP_NAME) + "-2"});
    QTreeWidgetItem *cursor = nullptr;
    if (_showDots && _topDirPrefix != DirPrefix) {
        auto *parent = new BrowseRow(*this, _list);
        parent->setData(0, Qt::UserRole, -1);
        parent->setText(0, "..");
        parent->setIcon(0, QFileIconProvider().icon(QFileIconProvider::Folder));
        if (focused == UString(".."))
            cursor = parent;
    }
    FOR_VECTOR(i, _items) {
        const CBrowseItem &bi = _items[i];
        const auto &fi = _files[bi.MainFileIndex];
        const UString name = fs2us(fi.Name);
        auto *row = new BrowseRow(*this, _list);
        row->setData(0, Qt::UserRole, int(i));
        row->setText(0, Text(name));
        row->setIcon(0, QFileIconProvider().icon(QFileInfo(Text(DirPrefix + name))));
        FILETIME ft;
        FiTime_To_FILETIME(fi.MTime, ft);
        wchar_t time[64] = {};
        if (!FILETIME_IsZero(ft))
            ConvertUtcFileTimeToString(ft, time, kTimestampPrintLevel_MIN);
        row->setText(1, QString::fromWCharArray(time));
        wchar_t size[64];
        wchar_t *tail = Browse_ConvertSizeToString(bi.Size, size);
        if (bi.WasInterrupted) {
            *tail++ = '+';
            *tail = 0;
        }
        row->setText(2, QString::fromWCharArray(size));
        if (fi.IsOsSymLink())
            row->setText(3, "Link");
        else if (bi.NumFiles != 0)
            row->setText(3, QString::number(bi.NumFiles) + (bi.WasInterrupted ? "+" : ""));
        if (bi.NumDirs != 0)
            row->setText(4, QString::number(bi.NumDirs) + (bi.WasInterrupted ? "+" : ""));
        if (bi.SubFileIndex >= 0)
            row->setText(5, Text(fs2us(_files[bi.SubFileIndex].Name)));
        for (int column : {2, 3, 4})
            row->setTextAlignment(column, Qt::AlignRight | Qt::AlignVCenter);
        const auto &owner = _owners[i];
        if (owner.active || owner.externallyProtected) {
            const QString reason =
                owner.active
                    ? QtLocalText("In use by an active operation or session",
                                  "Используется текущей операцией или сеансом")
                    : QtLocalText("Protected: external application completion is unconfirmed",
                                  "Защищён: завершение внешнего приложения не подтверждено");
            row->setToolTip(0, reason);
            row->setData(0, Qt::UserRole + 1, reason);
            row->setForeground(0, palette().color(QPalette::Disabled, QPalette::Text));
        }
        row->setSelected(selected.FindInSorted(name) != -1);
        if (focused == name && !cursor)
            cursor = row;
    }
    _list->sortItems(0, Qt::AscendingOrder);
    if (!cursor && _list->topLevelItemCount() > 0) {
        cursor = _list->topLevelItem(0);
        if (selected.IsEmpty())
            cursor->setSelected(true);
    }
    if (cursor) {
        _list->setCurrentItem(cursor, 0, QItemSelectionModel::NoUpdate);
        _list->scrollToItem(cursor);
    }
    _list->header()->setSortIndicator(_sortIndex,
                                      _ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
    _list->setUpdatesEnabled(true);
    UpdateDeleteAvailability();
}
int CBrowseDialog2::GetRealItemIndex(int row) const {
    auto *item = _list->topLevelItem(row);
    return item ? item->data(0, Qt::UserRole).toInt() : -1;
}
void CBrowseDialog2::GetSelected_RealIndexes(CUIntVector &vector) {
    vector.Clear();
    for (int row = 0; row < _list->topLevelItemCount(); ++row)
        if (_list->topLevelItem(row)->isSelected()) {
            const int realIndex = GetRealItemIndex(row);
            if (realIndex >= 0)
                vector.Add(unsigned(realIndex));
        }
}
void CBrowseDialog2::UpdateDeleteAvailability() {
    CUIntVector selected;
    GetSelected_RealIndexes(selected);
    bool enabled = selected.Size() != 0;
    FOR_VECTOR(i, selected) {
        const auto &owner = _owners[selected[i]];
        if (owner.active || owner.externallyProtected)
            enabled = false;
    }
    _delete->setEnabled(enabled);
}
bool CBrowseDialog2::eventFilter(QObject *object, QEvent *event) {
    if (object == _list && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        switch (key->key()) {
        case Qt::Key_Delete:
            OnDelete();
            return true;
        case Qt::Key_Backspace:
            OpenParentFolder();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            OnItemEnter();
            return true;
        case Qt::Key_F5:
            Reload_WithErrorMessage();
            return true;
        default:
            break;
        }
    }
    return QDialog::eventFilter(object, event);
}
