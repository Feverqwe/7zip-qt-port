#include "qtprogresscontrols.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "qtdialoglayout.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtdisclosure.h"
#include "qtlangcontrols.h"
#include "qtlistview.h"
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
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

void CQtProgressDialog::AddControl(unsigned id, const char *name, QWidget *widget) {
    widget->setObjectName(QString::fromLatin1(name));
    _controls.insert(id, widget);
}

CQtProgressDialog::CQtProgressDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setObjectName("progressDialog");
    const QtDialogUnits units(font());
    _unitX = units.horizontalScale();
    _clock.start();
    auto addLabel = [this](unsigned id, const char *name, const QString &text, bool right) {
        auto *label = new QLabel(text, this);
        label->setTextFormat(Qt::PlainText);
        label->setMinimumHeight(label->fontMetrics().height());
        label->setAlignment((right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignTop);
        AddControl(id, name, label);
    };
#define LABEL(id, text) addLabel(id, #id, QStringLiteral(text), false)
    LABEL(IDT_PROGRESS_ELAPSED, "Elapsed time:");
    LABEL(IDT_PROGRESS_REMAINING, "Remaining time:");
    LABEL(IDT_PROGRESS_FILES, "Files:");
    LABEL(IDT_PROGRESS_ERRORS, "Errors:");
    LABEL(IDT_PROGRESS_TOTAL, "Total size:");
    LABEL(IDT_PROGRESS_SPEED, "Speed:");
    LABEL(IDT_PROGRESS_PROCESSED, "Processed:");
    LABEL(IDT_PROGRESS_PACKED, "Compressed size:");
    LABEL(IDT_PROGRESS_RATIO, "Compression ratio:");
#undef LABEL
#define VALUE(id) addLabel(id, #id, QString(), true)
    VALUE(IDT_PROGRESS_ELAPSED_VAL);
    VALUE(IDT_PROGRESS_REMAINING_VAL);
    VALUE(IDT_PROGRESS_FILES_VAL);
    VALUE(IDT_PROGRESS_FILES_TOTAL);
    VALUE(IDT_PROGRESS_ERRORS_VAL);
    VALUE(IDT_PROGRESS_TOTAL_VAL);
    VALUE(IDT_PROGRESS_SPEED_VAL);
    VALUE(IDT_PROGRESS_PROCESSED_VAL);
    VALUE(IDT_PROGRESS_PACKED_VAL);
    VALUE(IDT_PROGRESS_RATIO_VAL);
#undef VALUE
    addLabel(IDT_PROGRESS_STATUS, "IDT_PROGRESS_STATUS", {}, false);
    addLabel(IDT_PROGRESS_FILE_NAME, "IDT_PROGRESS_FILE_NAME", {}, false);
    auto *bar = new QProgressBar(this);
    bar->setTextVisible(false);
    bar->setRange(0, 100);
    bar->setValue(0);
    AddControl(IDC_PROGRESS1, "IDC_PROGRESS1", bar);
    auto *messages = new CListView(this);
    messages->setHeaderHidden(true);
    messages->setColumnCount(2);
    messages->setTextElideMode(Qt::ElideNone);
    AddControl(IDL_PROGRESS_MESSAGES, "IDL_PROGRESS_MESSAGES", messages);
    auto *background = new QPushButton(QStringLiteral("&Background"), this);
    background->setAutoDefault(true);
    background->setDefault(true); // Source DEFPUSHBUTTON.
    AddControl(IDB_PROGRESS_BACKGROUND, "IDB_PROGRESS_BACKGROUND", background);
    auto *pause = new QPushButton(QStringLiteral("&Pause"), this);
    pause->setAutoDefault(true);
    AddControl(IDB_PAUSE, "IDB_PAUSE", pause);
    auto *cancel = new QPushButton(QStringLiteral("Cancel"), this);
    cancel->setAutoDefault(true);
    AddControl(kCancelID, "IDCANCEL", cancel);
    auto *buttons = new QDialogButtonBox(this);
    buttons->setObjectName("progressButtons");
    buttons->addButton(background, QDialogButtonBox::ActionRole);
    buttons->addButton(pause, QDialogButtonBox::ActionRole);
    buttons->addButton(cancel, QDialogButtonBox::RejectRole);
    auto *outer = QtCreateDialogLayout(*this, *buttons);
    auto *body = new QWidget(this);
    auto *content = new QVBoxLayout(body);
    // Match the other scrolling forms: reserve a gutter for native overlays.
    content->setContentsMargins(0, 0, style()->pixelMetric(QStyle::PM_ScrollBarExtent), 0);
    content->setSpacing(8);
    content->addWidget(GetItem(IDT_PROGRESS_STATUS));
    content->addWidget(bar);
    auto *file = static_cast<QLabel *>(GetItem(IDT_PROGRESS_FILE_NAME));
    file->setMinimumHeight(fontMetrics().height() * 2);
    file->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    content->addWidget(file);
    auto *mainStats = new QGridLayout;
    mainStats->setHorizontalSpacing(12);
    mainStats->setVerticalSpacing(8);
    mainStats->setColumnStretch(1, 1);
    const unsigned mainLabels[] = {IDT_PROGRESS_ELAPSED, IDT_PROGRESS_REMAINING, IDT_PROGRESS_FILES,
                                   IDT_PROGRESS_ERRORS};
    const unsigned mainValues[] = {IDT_PROGRESS_ELAPSED_VAL, IDT_PROGRESS_REMAINING_VAL,
                                   IDT_PROGRESS_FILES_VAL, IDT_PROGRESS_ERRORS_VAL};
    for (int row = 0; row < 4; ++row) {
        mainStats->addWidget(GetItem(mainLabels[row]), row, 0);
        if (row == 2) {
            auto *files = new QHBoxLayout;
            files->setSpacing(0);
            files->addStretch();
            files->addWidget(GetItem(IDT_PROGRESS_FILES_VAL));
            files->addWidget(GetItem(IDT_PROGRESS_FILES_TOTAL));
            mainStats->addLayout(files, row, 1);
        } else
            mainStats->addWidget(GetItem(mainValues[row]), row, 1);
    }
    content->addLayout(mainStats);
    auto *detailsBody = new QWidget(body);
    auto *stats = new QGridLayout(detailsBody);
    stats->setContentsMargins(24, 0, 0, 0);
    stats->setHorizontalSpacing(12);
    stats->setVerticalSpacing(8);
    stats->setColumnStretch(1, 1);
    const unsigned labels[] = {IDT_PROGRESS_TOTAL, IDT_PROGRESS_SPEED, IDT_PROGRESS_PROCESSED,
                               IDT_PROGRESS_PACKED, IDT_PROGRESS_RATIO};
    const unsigned values[] = {IDT_PROGRESS_TOTAL_VAL, IDT_PROGRESS_SPEED_VAL,
                               IDT_PROGRESS_PROCESSED_VAL, IDT_PROGRESS_PACKED_VAL,
                               IDT_PROGRESS_RATIO_VAL};
    for (int row = 0; row < 5; ++row) {
        stats->addWidget(GetItem(labels[row]), row, 0);
        stats->addWidget(GetItem(values[row]), row, 1);
    }
    auto *details = new QtDisclosure(detailsBody, body);
    details->setObjectName("progressDetailsSection");
    details->button()->setObjectName("progressDetailsToggle");
    QtBindLocalText(details->button(), "DialogRedesign",
                    QT_TRANSLATE_NOOP("DialogRedesign", "Details"));
    content->addWidget(details);
    content->addStretch();
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("progressControlsScroll");
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setFocusPolicy(Qt::NoFocus);
    scroll->setWidgetResizable(true);
    scroll->setWidget(body);
    // Keep the overview reachable when the independent error list appears.
    // Expanding Details scrolls within this area instead of moving the footer.
    scroll->setMinimumHeight(body->minimumSizeHint().height());
    outer->addWidget(scroll, 1);
    messages->setMinimumHeight(fontMetrics().height() * 4);
    outer->addWidget(messages, 1);
    GetItem(IDT_PROGRESS_STATUS)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    setTabOrder(details->button(), messages);
    setTabOrder(messages, background);
    setTabOrder(background, pause);
    setTabOrder(pause, cancel);
    GetItem(IDT_PROGRESS_ERRORS)->hide();
    GetItem(IDT_PROGRESS_ERRORS_VAL)->hide();
    messages->hide();
    resize(QSize(560, minimumSizeHint().height()).expandedTo(minimumSizeHint()));
}

QWidget *CQtProgressDialog::GetItem(unsigned id) const { return _controls.value(id); }
qintptr CQtProgressDialog::CreateModal(QWidget *parent) {
    if (parentWidget() != parent)
        setParent(parent, windowFlags());
    return exec();
}
void CQtProgressDialog::End(qintptr result) { done(int(result)); }
void CQtProgressDialog::SetDefaultButton(unsigned id) {
    auto *button = static_cast<QPushButton *>(GetItem(id));
    for (auto *other : findChildren<QPushButton *>())
        other->setDefault(false);
    button->setEnabled(true);
    button->setAutoDefault(true);
    button->setDefault(true);
    button->setFocus();
}
void CQtProgressDialog::MessageBox(QWidget *parent, const UString &message, const UString &title,
                                   bool error) {
    QMessageBox box(error ? QMessageBox::Critical : QMessageBox::NoIcon,
                    QString::fromWCharArray(title.Ptr()), QString::fromWCharArray(message.Ptr()),
                    QMessageBox::Ok, parent);
    box.setObjectName("progressFinalMessage");
    box.setTextFormat(Qt::PlainText);
    box.exec();
}
int CQtProgressDialog::AskCancel(QWidget *parent, const UString &message, const UString &title) {
    QMessageBox box(QMessageBox::NoIcon, QString::fromWCharArray(title.Ptr()),
                    QString::fromWCharArray(message.Ptr()),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, parent);
    box.setObjectName("progressCancelQuestion");
    box.setTextFormat(Qt::PlainText);
    box.setDefaultButton(QMessageBox::Yes);
    box.setEscapeButton(QMessageBox::Cancel);
    const int result = box.exec();
    if (result == QMessageBox::Yes)
        return 6; // Native IDYES.
    if (result == QMessageBox::No)
        return 7; // Native IDNO.
    return 2;     // Native IDCANCEL, including Escape/window close.
}
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
    QtNormalizeDialogSize(*this, fullNormalize);
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
