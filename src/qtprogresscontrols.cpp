#include "qtprogresscontrols.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "qtlistview.h"
#include <QApplication>
#include <QClipboard>
#include <QFontMetricsF>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <cmath>

namespace {
// The standard resource IDCANCEL is 2; the port does not include Win32 controls.
constexpr unsigned kCancelID = 2;
} // namespace

void CQtProgressBar::Attach(QWidget *widget) { _view = static_cast<QProgressBar *>(widget); }
void CQtProgressBar::SetRange32(int minimum, int maximum) { _view->setRange(minimum, maximum); }
void CQtProgressBar::SetPos(int position) {
    // PBM_SETPOS clamps to the nearest boundary; QProgressBar ignores an
    // out-of-range value. Preserve the source control service's behavior.
    _view->setValue(qBound(_view->minimum(), position, _view->maximum()));
}

void CQtProgressListView::Attach(QWidget *widget) { _view = static_cast<CListView *>(widget); }
void CQtProgressListView::InsertColumn(unsigned column, const wchar_t *text, int width) {
    if (_view->columnCount() <= int(column))
        _view->setColumnCount(int(column) + 1);
    _view->headerItem()->setText(int(column), QString::fromWCharArray(text));
    _view->setColumnWidth(int(column), width);
}
int CQtProgressListView::InsertItem(unsigned index, const UString &text) {
    _view->InsertItem(index, text);
    return _view->indexOfTopLevelItem(_view->topLevelItem(int(index)));
}
void CQtProgressListView::SetSubItem(unsigned index, int column, const UString &text) {
    _view->SetSubItem(index, column, text);
}
void CQtProgressListView::SetColumnWidthAuto(int column) { _view->SetColumnWidthAuto(column); }
void CQtProgressListView::Move(int x, int y, int width, int height) {
    _view->setGeometry(x, y, width, height);
}
int CQtProgressListView::GetNextSelectedItem(int index) const {
    return _view->GetNextSelectedItem(index);
}
int CQtProgressListView::GetItemCount() const { return _view->topLevelItemCount(); }

void CQtProgressDialog::AddControl(unsigned id, const char *name, QWidget *widget, int x, int y,
                                   int width, int height) {
    widget->setObjectName(QString::fromLatin1(name));
    widget->setGeometry(int(std::round(x * _unitX)), int(std::round(y * _unitY)),
                        int(std::round(width * _unitX)), int(std::round(height * _unitY)));
    _controls.insert(id, widget);
}

CQtProgressDialog::CQtProgressDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("progressDialog");
    const QFontMetricsF metrics(font());
    // Win32 dialog units: average character width / 4 and font height / 8.
    _unitX = metrics.horizontalAdvance(
                 QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")) /
             52 / 4;
    // QLabel rounds its line height upward; fractional font metrics must not
    // produce a control one pixel shorter than its rendered text.
    _unitY = std::ceil(metrics.height()) / 8;
    _clock.start();
    auto addLabel = [this](unsigned id, const char *name, const QString &text, int x, int y,
                           int width, int height, bool right) {
        auto *label = new QLabel(text, this);
        label->setTextFormat(Qt::PlainText);
        label->setAlignment((right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignTop);
        AddControl(id, name, label, x, y, width, height);
    };
#define LABEL(id, text, x, y) addLabel(id, #id, QStringLiteral(text), x, y, 90, 8, false)
    LABEL(IDT_PROGRESS_ELAPSED, "Прошло времени:", 8, 8);
    LABEL(IDT_PROGRESS_REMAINING, "Осталось времени:", 8, 19);
    LABEL(IDT_PROGRESS_FILES, "Файлы:", 8, 30);
    LABEL(IDT_PROGRESS_ERRORS, "Ошибки:", 8, 52);
    LABEL(IDT_PROGRESS_TOTAL, "Общий размер:", 206, 8);
    LABEL(IDT_PROGRESS_SPEED, "Скорость:", 206, 19);
    LABEL(IDT_PROGRESS_PROCESSED, "Обработано:", 206, 30);
    LABEL(IDT_PROGRESS_PACKED, "Сжатый размер:", 206, 41);
    LABEL(IDT_PROGRESS_RATIO, "Степень сжатия:", 206, 52);
#undef LABEL
#define VALUE(id, x, y) addLabel(id, #id, QString(), x, y, 72, 8, true)
    VALUE(IDT_PROGRESS_ELAPSED_VAL, 98, 8);
    VALUE(IDT_PROGRESS_REMAINING_VAL, 98, 19);
    VALUE(IDT_PROGRESS_FILES_VAL, 98, 30);
    VALUE(IDT_PROGRESS_FILES_TOTAL, 98, 41);
    VALUE(IDT_PROGRESS_ERRORS_VAL, 98, 52);
    VALUE(IDT_PROGRESS_TOTAL_VAL, 296, 8);
    VALUE(IDT_PROGRESS_SPEED_VAL, 296, 19);
    VALUE(IDT_PROGRESS_PROCESSED_VAL, 296, 30);
    VALUE(IDT_PROGRESS_PACKED_VAL, 296, 41);
    VALUE(IDT_PROGRESS_RATIO_VAL, 296, 52);
#undef VALUE
    addLabel(IDT_PROGRESS_STATUS, "IDT_PROGRESS_STATUS", {}, 8, 64, 360, 8, false);
    addLabel(IDT_PROGRESS_FILE_NAME, "IDT_PROGRESS_FILE_NAME", {}, 8, 76, 360, 24, false);
    auto *bar = new QProgressBar(this);
    bar->setTextVisible(false);
    bar->setRange(0, 100);
    bar->setValue(0);
    AddControl(IDC_PROGRESS1, "IDC_PROGRESS1", bar, 8, 100, 360, 16);
    auto *messages = new CListView(this);
    messages->setHeaderHidden(true);
    messages->setColumnCount(2);
    messages->setTextElideMode(Qt::ElideNone);
    AddControl(IDL_PROGRESS_MESSAGES, "IDL_PROGRESS_MESSAGES", messages, 8, 124, 360, 48);
    auto *background = new QPushButton(QStringLiteral("В &фоновый режим"), this);
    background->setAutoDefault(false);
    background->setEnabled(false); // Mandatory priority service follows in b2b2b.
    AddControl(IDB_PROGRESS_BACKGROUND, "IDB_PROGRESS_BACKGROUND", background, 112, 180, 80, 16);
    auto *pause = new QPushButton(QStringLiteral("&Пауза"), this);
    pause->setAutoDefault(false);
    AddControl(IDB_PAUSE, "IDB_PAUSE", pause, 200, 180, 80, 16);
    auto *cancel = new QPushButton(QStringLiteral("Отмена"), this);
    cancel->setAutoDefault(false);
    cancel->setEnabled(false); // Mandatory modal cancel/worker lifecycle follows in b3.
    AddControl(kCancelID, "IDCANCEL", cancel, 288, 180, 80, 16);
    setTabOrder(messages, background);
    setTabOrder(background, pause);
    setTabOrder(pause, cancel);
    resize(int(std::round(376 * _unitX)), int(std::round(204 * _unitY)));
}

QWidget *CQtProgressDialog::GetItem(unsigned id) const { return _controls.value(id); }
#ifdef _WIN32
HWND CQtProgressDialog::GetNativeTaskbarWindow(QWidget *widget) {
    return widget ? reinterpret_cast<HWND>(widget->winId()) : nullptr;
}
#endif
void CQtProgressDialog::SetItemText(unsigned id, const UString &text) {
    QWidget *widget = GetItem(id);
    if (auto *label = qobject_cast<QLabel *>(widget))
        label->setText(ToQString(text));
    else if (auto *button = qobject_cast<QPushButton *>(widget))
        button->setText(ToQString(text));
}
void CQtProgressDialog::GetItemText(unsigned id, UString &text) const {
    QWidget *widget = GetItem(id);
    if (auto *label = qobject_cast<QLabel *>(widget))
        text = ToUString(label->text());
    else if (auto *button = qobject_cast<QPushButton *>(widget))
        text = ToUString(button->text());
}
void CQtProgressDialog::SetText(const UString &text) { setWindowTitle(ToQString(text)); }
void CQtProgressDialog::ShowItem_Bool(unsigned id, bool visible) {
    GetItem(id)->setVisible(visible);
}
void CQtProgressDialog::HideItem(unsigned id) { ShowItem_Bool(id, false); }
void CQtProgressDialog::GetItemSizes(unsigned id, int &width, int &height) const {
    width = GetItem(id)->width();
    height = GetItem(id)->height();
}
void CQtProgressDialog::GetClientRectOfItem(unsigned id, CQtProgressRect &rect) const {
    const QRect geometry = GetItem(id)->geometry();
    rect = {geometry.x(), geometry.y(), geometry.x() + geometry.width(),
            geometry.y() + geometry.height()};
}
void CQtProgressDialog::MoveItem(unsigned id, int x, int y, int width, int height) {
    GetItem(id)->setGeometry(x, y, width, height);
}
int CQtProgressDialog::Units_To_Pixels_X(int units) const {
    return int(std::round(units * _unitX));
}
UInt32 CQtProgressDialog::GetTickCount() const { return UInt32(_clock.elapsed()); }
void CQtProgressDialog::ChangeSubWindowSizeX(QWidget *widget, int width) {
    widget->resize(width, widget->height());
}
void CQtProgressDialog::ClipboardSetText(const UString &text) {
    QApplication::clipboard()->setText(ToQString(text));
}
void CQtProgressDialog::NormalizeSize(bool fullNormalize) {
    const QRect workRect = screen()->availableGeometry();
    const QRect rect = frameGeometry();
    if (!rect.intersects(workRect) ||
        (rect.width() <= workRect.width() && rect.height() <= workRect.height()))
        return;
    if (fullNormalize) {
        showMaximized();
        return;
    }
    QRect normalized = rect;
    normalized.setSize(rect.size().boundedTo(workRect.size()));
    normalized.moveCenter(workRect.center());
    setGeometry(normalized);
}

QIcon CQtProgressDialog::LoadIcon(int id) {
    // Static-library resources need an explicit reference to their initializer.
    static const bool initialized = [] {
        Q_INIT_RESOURCE(upstream_progress_icons);
        return true;
    }();
    Q_UNUSED(initialized)
    if (id == IDI_ICON)
        return QIcon(QStringLiteral(":/upstream/7zip/UI/FileManager/FM.ico"));
    return {};
}
void CQtProgressDialog::SetIcon(const QIcon &icon) { setWindowIcon(icon); }
