#include "qtpaneladdresscombo.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "Windows/FileName.h"
#include "qtvolumemounts.h"
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTimer>

namespace {
constexpr int kIndentRole = Qt::UserRole + 12;
class AddressItemDelegate final : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QStyleOptionViewItem indented(option);
        indented.rect.adjust(index.data(kIndentRole).toInt() * option.fontMetrics.height(), 0, 0,
                             0);
        QStyledItemDelegate::paint(painter, indented, index);
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.rwidth() += index.data(kIndentRole).toInt() * option.fontMetrics.height();
        return size;
    }
};
} // namespace

QtPanelAddressCombo::QtPanelAddressCombo(CPanel &panel,
                                         std::function<HRESULT(const QString &, bool)> navigate,
                                         QWidget *parent)
    : QComboBox(parent), panel_(panel), navigate_(std::move(navigate)) {
    setEditable(true);
    lineEdit()->installEventFilter(this);
    setItemDelegate(new AddressItemDelegate(this));
    connect(this, &QComboBox::activated, this,
            [this](int) { panel_.OnComboBoxCommand(QtAddressCommand::SelectionAccepted, *this); });
    connect(lineEdit(), &QLineEdit::returnPressed, this, [this] {
        bool result;
        panel_.OnNotifyComboBoxEndEdit(QtAddressEditReason::Return, *this, result);
    });
}

void QtPanelAddressCombo::showPopup() {
    const QString text = currentText();
    {
        const QSignalBlocker blocker(this);
        panel_.LoadFullPath();
        panel_.OnComboBoxCommand(QtAddressCommand::DropDown, *this);
        setCurrentIndex(-1);
        lineEdit()->setText(text);
    }
    QComboBox::showPopup();
}

void QtPanelAddressCombo::ResetContent() { clear(); }
void QtPanelAddressCombo::InsertItem(const UString &name, int iconIndex, unsigned indent,
                                     bool namespaceItem) {
    QIcon icon = g_Ext_to_Icon_Map.GetIcon(iconIndex);
    if (namespaceItem) {
        const QStyle::StandardPixmap kind =
            name == LangString(IDS_COMPUTER)    ? QStyle::SP_ComputerIcon
            : name == LangString(IDS_DOCUMENTS) ? QStyle::SP_DirHomeIcon
            : name == LangString(IDS_NETWORK)   ? QStyle::SP_DriveNetIcon
                                                : QStyle::SP_DriveHDIcon;
        icon = style()->standardIcon(kind);
    }
    addItem(icon, name.IsEmpty() ? QStringLiteral("/") : ToQString(name));
    setItemData(count() - 1, indent, kIndentRole);
}
int QtPanelAddressCombo::GetCurSel() const { return currentIndex(); }
void QtPanelAddressCombo::SetCurSel(int index) {
    const QString text = currentText();
    setCurrentIndex(index);
    lineEdit()->setText(text);
}
int QtPanelAddressCombo::GetIconIndex(const UString &path, DWORD attributes) {
    return g_Ext_to_Icon_Map.GetIconIndex(attributes, path);
}
void QtPanelAddressCombo::GetLogicalDriveStrings(FStringVector &paths) {
#ifdef _WIN32
    NWindows::NFile::NFind::MyGetLogicalDriveStrings(paths);
#else
    paths.Clear();
    for (const auto &mount : QtSystemVolumeMounts()) {
        UString path = ToUString(mount.path);
        NWindows::NFile::NName::NormalizeDirPathPrefix(path);
        paths.Add(us2fs(path));
    }
#endif
}
UString QtPanelAddressCombo::GetText() const { return ToUString(currentText()); }
void QtPanelAddressCombo::SetText(const UString &text) { lineEdit()->setText(ToQString(text)); }
HRESULT QtPanelAddressCombo::BindToPathAndRefresh(const UString &path, bool sourcePath) {
    return navigate_(ToQString(path), sourcePath);
}
void QtPanelAddressCombo::SetFocusToList() {
    QTimer::singleShot(0, this, [this] {
        // Qt can deliver a later synchronous panel focus before this posted
        // request. Preserve that newer interaction instead of retargeting it.
        QWidget *focus = QApplication::focusWidget();
        if (focus && parentWidget()->isAncestorOf(focus))
            panel_.SetFocusToList();
    });
}

bool QtPanelAddressCombo::eventFilter(QObject *object, QEvent *event) {
    if (object == lineEdit()) {
        if (event->type() == QEvent::KeyPress &&
            static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            bool result;
            return panel_.OnNotifyComboBoxEndEdit(QtAddressEditReason::Escape, *this, result);
        }
    }
    return QComboBox::eventFilter(object, event);
}
