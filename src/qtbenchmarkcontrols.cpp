#include "qtbenchmarkcontrols.h"
#include "7zip/UI/GUI/BenchmarkDialogRes.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtlangcontrols.h"
#include "qtmodalresult.h"
#include <QApplication>
#include <QCloseEvent>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>

namespace {
class CBenchmarkControlsScroll final : public QScrollArea {
  public:
    using QScrollArea::QScrollArea;
    QSize minimumSizeHint() const override {
        QSize size = QScrollArea::minimumSizeHint();
        size.setWidth(sizeHint().width());
        return size;
    }
    QSize sizeHint() const override {
        QSize size = QScrollArea::sizeHint();
        const int controls = widget() ? widget()->minimumSizeHint().width() : 0;
        size.setWidth(qMax(QtDialogUnits(font()).pixels(QSize(332, 0)).width(), controls) +
                      qMax(style()->pixelMetric(QStyle::PM_ScrollBarExtent),
                           verticalScrollBar()->sizeHint().width()));
        return size;
    }
};
} // namespace

QString CQtBenchmarkWindow::ResourceCaption(unsigned id) {
    // Original BenchmarkDialog.rc captions are not STRINGTABLE entries.
    static const QHash<unsigned, QString> captions = {
        {IDD_BENCH, "Benchmark"},
        {IDT_BENCH_DICTIONARY, "&Dictionary size:"},
        {IDT_BENCH_MEMORY, "Memory usage:"},
        {IDT_BENCH_NUM_THREADS, "&Number of CPU threads:"},
        {IDT_BENCH_SIZE, "Size"},
        {IDT_BENCH_SPEED, "Speed"},
        {IDT_BENCH_USAGE_LABEL, "CPU Usage"},
        {IDT_BENCH_RPU_LABEL, "Rating / Usage"},
        {IDT_BENCH_RATING_LABEL, "Rating"},
        {IDG_BENCH_COMPRESSING, "Compressing"},
        {IDG_BENCH_DECOMPRESSING, "Decompressing"},
        {IDG_BENCH_TOTAL_RATING, "Total Rating"},
        {IDT_BENCH_CURRENT, "Current"},
        {IDT_BENCH_RESULTING, "Resulting"},
        {IDT_BENCH_ELAPSED, "Elapsed time:"},
        {IDT_BENCH_PASSES, "Passes:"},
        {IDB_RESTART, "&Restart"},
        {IDB_STOP, "&Stop"},
        {9, "&Help"},
        {2, "Cancel"}};
    return captions.value(id);
}
QString CQtBenchmarkWindow::QtLangText(unsigned id) {
    return ::QtLangText(id, ResourceCaption(id));
}

CQtBenchmarkWindow::CQtBenchmarkWindow(QWidget *parent) : QDialog(parent) {
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    clock_.start();
    QtBindResourceText(this, IDD_BENCH, ResourceCaption(IDD_BENCH));
    setObjectName("IDD_BENCH");
}
QWidget *CQtBenchmarkWindow::Register(unsigned id, const char *name, QWidget *widget) {
    controls_.insert(id, widget);
    widget->setObjectName(QString::fromLatin1(name));
    unsigned captionId = id;
    if (id == IDT_BENCH_CURRENT + 50 || id == IDT_BENCH_RESULTING + 50)
        captionId -= 50;
    const QString caption = ResourceCaption(captionId);
    if (id == IDT_BENCH_SPEED)
        widget->setProperty("languageRemoveColon", true);
    if (!caption.isEmpty()) {
        if (id == IDCANCEL || id == IDHELP)
            QtBindStandardResourceText(widget, id, caption);
        else
            QtBindResourceText(widget, captionId, caption);
    }
    return widget;
}
QLabel *CQtBenchmarkWindow::label(unsigned id, const char *name, const QString &text) {
    auto *view = new QLabel(text, this);
    Register(id, name, view);
    view->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return view;
}
QLabel *CQtBenchmarkWindow::rightLabel(unsigned id, const char *name, const QString &text) {
    auto *view = label(id, name, text);
    view->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return view;
}
QPushButton *CQtBenchmarkWindow::button(unsigned id, const char *name, const QString &text) {
    auto *view = new QPushButton(text, this);
    Register(id, name, view);
    view->setFixedWidth(
        qMax(QtDialogUnits(font()).pixels(QSize(64, 0)).width(), view->sizeHint().width()));
    connect(view, &QPushButton::clicked, this, [this, id, view] {
        if (id == 2)
            OnCancel();
        else if (id == 9)
            OnHelp();
        else
            OnButtonClicked(id, view);
    });
    return view;
}
QComboBox *CQtBenchmarkWindow::combo(unsigned id, const char *name) {
    auto *view = new QComboBox(this);
    Register(id, name, view);
    connect(view, &QComboBox::activated, this, [this, id](int) { OnCommand(1, id, 0); });
    return view;
}
void CQtBenchmarkWindow::BuildControls(bool totalMode) {
    auto *outer = new QVBoxLayout(this);
    if (totalMode) {
        setObjectName("IDD_BENCH_TOTAL");
        auto *elapsed = new QHBoxLayout;
        elapsed->addWidget(
            label(IDT_BENCH_ELAPSED, "IDT_BENCH_ELAPSED", QtLangText(IDT_BENCH_ELAPSED)));
        elapsed->addWidget(rightLabel(IDT_BENCH_ELAPSED_VAL, "IDT_BENCH_ELAPSED_VAL"));
        elapsed->addStretch();
        outer->addLayout(elapsed);
        auto *console = new QPlainTextEdit(this);
        console->setReadOnly(true);
        console->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        Register(IDE_BENCH2_EDIT, "IDE_BENCH2_EDIT", console);
        outer->addWidget(console, 1);
        outer->addLayout(BuildFooter());
    } else {
        auto *body = new QHBoxLayout;
        BuildNormalControls(body);
        outer->addLayout(body, 1);
    }
    resize(QtDialogUnits(font())
               .pixels(totalMode ? QSize(376, 276) : QSize(488, 264))
               .expandedTo(minimumSizeHint()));
}
QHBoxLayout *CQtBenchmarkWindow::BuildFooter() {
    auto *footer = new QHBoxLayout;
    footer->setSpacing(QtDialogUnits(font()).pixels(QSize(8, 0)).width());
    footer->addStretch();
    footer->addWidget(button(9, "IDHELP", QtLangText(9)));
    footer->addWidget(button(2, "IDCANCEL", QtLangText(2)));
    return footer;
}
QGridLayout *CQtBenchmarkWindow::BuildParameters() {
    auto *grid = new QGridLayout;
    grid->setVerticalSpacing(QtDialogUnits(font()).pixels(QSize(0, 6)).height());
    grid->addWidget(
        label(IDT_BENCH_DICTIONARY, "IDT_BENCH_DICTIONARY", QtLangText(IDT_BENCH_DICTIONARY)), 0,
        0);
    grid->addWidget(combo(IDC_BENCH_DICTIONARY, "IDC_BENCH_DICTIONARY"), 0, 1);
    auto *memory = new QVBoxLayout;
    memory->setSpacing(0);
    memory->addWidget(label(IDT_BENCH_MEMORY, "IDT_BENCH_MEMORY", QtLangText(IDT_BENCH_MEMORY)));
    memory->addWidget(label(IDT_BENCH_MEMORY_VAL, "IDT_BENCH_MEMORY_VAL"));
    grid->addLayout(memory, 0, 2);
    grid->addWidget(button(IDB_RESTART, "IDB_RESTART", QtLangText(IDB_RESTART)), 0, 3);
    grid->addWidget(
        label(IDT_BENCH_NUM_THREADS, "IDT_BENCH_NUM_THREADS", QtLangText(IDT_BENCH_NUM_THREADS)), 1,
        0);
    grid->addWidget(combo(IDC_BENCH_NUM_THREADS, "IDC_BENCH_NUM_THREADS"), 1, 1);
    grid->addWidget(label(IDT_BENCH_HARDWARE_THREADS, "IDT_BENCH_HARDWARE_THREADS"), 1, 2);
    grid->addWidget(button(IDB_STOP, "IDB_STOP", QtLangText(IDB_STOP)), 1, 3);
    qobject_cast<QLabel *>(GetItem(IDT_BENCH_DICTIONARY))->setBuddy(GetItem(IDC_BENCH_DICTIONARY));
    qobject_cast<QLabel *>(GetItem(IDT_BENCH_NUM_THREADS))
        ->setBuddy(GetItem(IDC_BENCH_NUM_THREADS));
    return grid;
}
void CQtBenchmarkWindow::BuildNormalControls(QHBoxLayout *body) {
    auto *left = new QWidget(this);
    auto *grid = new QGridLayout(left);
    grid->setContentsMargins(0, 0, style()->pixelMetric(QStyle::PM_ScrollBarExtent), 0);
    grid->addLayout(BuildParameters(), 0, 0, 1, 2);
    grid->addLayout(BuildMetrics(), 1, 0, 1, 2);
    auto *error = rightLabel(IDT_BENCH_ERROR_MESSAGE, "IDT_BENCH_ERROR_MESSAGE");
    error->setWordWrap(true);
    grid->addWidget(error, 2, 0, 1, 2, Qt::AlignRight);
    grid->addLayout(BuildSummary(), 3, 0, 1, 2);
    const unsigned info[] = {IDT_BENCH_CPU, IDT_BENCH_CPU_FEATURE, IDT_BENCH_VER, IDT_BENCH_SYS1,
                             IDT_BENCH_SYS2};
    const char *names[] = {"IDT_BENCH_CPU", "IDT_BENCH_CPU_FEATURE", "IDT_BENCH_VER",
                           "IDT_BENCH_SYS1", "IDT_BENCH_SYS2"};
    for (int i = 0; i < 5; ++i) {
        auto *view = label(info[i], names[i]);
        if (i >= 3)
            view->hide();
        view->setWordWrap(true);
        if (i == 0 || i == 2)
            view->setAlignment(Qt::AlignRight | Qt::AlignTop);
        if (i == 0)
            grid->addWidget(view, 4, 0, 1, 2, Qt::AlignRight);
        else if (i == 1 || i == 2) {
            view->setAlignment((i == 2 ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignTop);
            grid->addWidget(view, 5, i - 1);
        } else
            grid->addWidget(view, i + 3, 0, 1, 2);
    }
    grid->setColumnStretch(0, 232);
    grid->setColumnStretch(1, 100);
    grid->setRowStretch(8, 1);
    auto *scroll = new CBenchmarkControlsScroll(this);
    scroll->setObjectName("benchmarkControlsScroll");
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setFocusPolicy(Qt::NoFocus);
    scroll->setWidgetResizable(true);
    scroll->setWidget(left);
    auto *pane = new QWidget(this);
    auto *column = new QVBoxLayout(pane);
    column->setContentsMargins(0, 0, 0, 0);
    column->addWidget(scroll, 1);
    column->addLayout(BuildFooter());
    // Compact Qt viewports scroll the controls and retain the footer. Give
    // useful metrics the available width until there is actual log output.
    body->addWidget(pane, 3);
    auto *log = label(IDT_BENCH_LOG, "IDT_BENCH_LOG");
    log->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    log->setTextFormat(Qt::PlainText);
    log->setTextInteractionFlags(Qt::NoTextInteraction);
    log->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    log->setMinimumWidth(QtDialogUnits(font()).pixels(QSize(20, 0)).width());
    log->hide();
    body->addWidget(log, 1);
}
QGridLayout *CQtBenchmarkWindow::BuildMetrics() {
    const unsigned captions[] = {IDT_BENCH_SIZE, IDT_BENCH_SPEED, IDT_BENCH_USAGE_LABEL,
                                 IDT_BENCH_RPU_LABEL, IDT_BENCH_RATING_LABEL};
    const char *captionNames[] = {"IDT_BENCH_SIZE", "IDT_BENCH_SPEED", "IDT_BENCH_USAGE_LABEL",
                                  "IDT_BENCH_RPU_LABEL", "IDT_BENCH_RATING_LABEL"};
    const unsigned values[2][2][5] = {
        {{IDT_BENCH_COMPRESS_SIZE1, IDT_BENCH_COMPRESS_SPEED1, IDT_BENCH_COMPRESS_USAGE1,
          IDT_BENCH_COMPRESS_RPU1, IDT_BENCH_COMPRESS_RATING1},
         {IDT_BENCH_COMPRESS_SIZE2, IDT_BENCH_COMPRESS_SPEED2, IDT_BENCH_COMPRESS_USAGE2,
          IDT_BENCH_COMPRESS_RPU2, IDT_BENCH_COMPRESS_RATING2}},
        {{IDT_BENCH_DECOMPR_SIZE1, IDT_BENCH_DECOMPR_SPEED1, IDT_BENCH_DECOMPR_USAGE1,
          IDT_BENCH_DECOMPR_RPU1, IDT_BENCH_DECOMPR_RATING1},
         {IDT_BENCH_DECOMPR_SIZE2, IDT_BENCH_DECOMPR_SPEED2, IDT_BENCH_DECOMPR_USAGE2,
          IDT_BENCH_DECOMPR_RPU2, IDT_BENCH_DECOMPR_RATING2}}};
    const int widths[] = {50, 52, 60, 46, 58, 58};
    const QtDialogUnits units(font());
    auto *metrics = new QGridLayout;
    auto *headers = new QGridLayout;
    headers->setHorizontalSpacing(0);
    for (int col = 0; col < 6; ++col)
        headers->setColumnMinimumWidth(col, units.pixels(QSize(widths[col], 0)).width());
    for (int col = 0; col < 5; ++col) {
        auto *header = rightLabel(captions[col], captionNames[col], QtLangText(captions[col]));
        header->setWordWrap(true);
        header->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        headers->addWidget(header, 0, col + 1);
    }
    metrics->addLayout(headers, 0, 0);
    for (int group = 0; group < 2; ++group) {
        const unsigned id = group ? IDG_BENCH_DECOMPRESSING : IDG_BENCH_COMPRESSING;
        auto *box = new QGroupBox(this);
        Register(id, group ? "IDG_BENCH_DECOMPRESSING" : "IDG_BENCH_COMPRESSING", box);
        auto *rows = new QGridLayout(box);
        rows->setHorizontalSpacing(0);
        for (int col = 0; col < 6; ++col)
            rows->setColumnMinimumWidth(col, units.pixels(QSize(widths[col], 0)).width());
        box->ensurePolished();
        if (group == 0) {
            const auto layoutMargins = rows->contentsMargins();
            const auto frameMargins = box->contentsMargins();
            headers->setContentsMargins(layoutMargins.left() + frameMargins.left(), 0,
                                        layoutMargins.right() + frameMargins.right(), 0);
        }
        metrics->addWidget(box, group + 1, 0);
        for (int row = 0; row < 2; ++row) {
            const unsigned textId = row ? IDT_BENCH_RESULTING : IDT_BENCH_CURRENT;
            const unsigned fieldId = group ? textId + 50 : textId;
            rows->addWidget(label(fieldId,
                                  group ? (row ? "IDT_BENCH_RESULTING2" : "IDT_BENCH_CURRENT2")
                                        : (row ? "IDT_BENCH_RESULTING" : "IDT_BENCH_CURRENT"),
                                  QtLangText(textId)),
                            row, 0);
            for (int col = 0; col < 5; ++col) {
                const unsigned valueId = values[group][row][col];
                const QByteArray name = "benchmarkValue" + QByteArray::number(valueId);
                auto *view = rightLabel(valueId, name.constData());
                view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
                rows->addWidget(view, row, col + 1);
            }
        }
    }
    return metrics;
}
QGridLayout *CQtBenchmarkWindow::BuildSummary() {
    auto *summary = new QGridLayout;
    summary->addWidget(label(IDT_BENCH_ELAPSED, "IDT_BENCH_ELAPSED", QtLangText(IDT_BENCH_ELAPSED)),
                       0, 0);
    summary->addWidget(rightLabel(IDT_BENCH_ELAPSED_VAL, "IDT_BENCH_ELAPSED_VAL"), 0, 1);
    summary->addWidget(label(IDT_BENCH_PASSES, "IDT_BENCH_PASSES", QtLangText(IDT_BENCH_PASSES)), 1,
                       0);
    summary->addWidget(rightLabel(IDT_BENCH_PASSES_VAL, "IDT_BENCH_PASSES_VAL"), 1, 1);
    summary->addWidget(combo(IDC_BENCH_NUM_PASSES, "IDC_BENCH_NUM_PASSES"), 2, 0);
    auto *total = new QGroupBox(QtLangText(IDG_BENCH_TOTAL_RATING), this);
    Register(IDG_BENCH_TOTAL_RATING, "IDG_BENCH_TOTAL_RATING", total);
    auto *totalLayout = new QHBoxLayout(total);
    totalLayout->addWidget(rightLabel(IDT_BENCH_TOTAL_USAGE_VAL, "IDT_BENCH_TOTAL_USAGE_VAL"));
    totalLayout->addWidget(rightLabel(IDT_BENCH_TOTAL_RPU_VAL, "IDT_BENCH_TOTAL_RPU_VAL"));
    totalLayout->addWidget(rightLabel(IDT_BENCH_TOTAL_RATING_VAL, "IDT_BENCH_TOTAL_RATING_VAL"));
    summary->addWidget(total, 0, 2, 3, 1);
    return summary;
}
void CQtBenchmarkWindow::SetItemText(unsigned id, const UString &text) {
    const QString value = QString::fromWCharArray(text.Ptr());
    if (auto *label = qobject_cast<QLabel *>(GetItem(id))) {
        label->setText(value);
        if (id == IDT_BENCH_LOG || id == IDT_BENCH_SYS1 || id == IDT_BENCH_SYS2)
            label->setVisible(!value.isEmpty());
    } else if (auto *edit = qobject_cast<QPlainTextEdit *>(GetItem(id)))
        edit->setPlainText(value);
}
void CQtBenchmarkWindow::SetItemTextA(unsigned id, const AString &text) {
    SetItemText(id, UString(QString::fromUtf8(text.Ptr()).toStdWString().c_str()));
}
void CQtBenchmarkWindow::GetItemText(unsigned id, UString &text) const {
    if (const auto *label = qobject_cast<QLabel *>(GetItem(id)))
        text = label->text().toStdWString().c_str();
}
void CQtBenchmarkWindow::EnableItem(unsigned id, bool enabled) {
    if (auto *view = GetItem(id))
        view->setEnabled(enabled);
}
QWidget *CQtBenchmarkWindow::GetFocus() const { return QApplication::focusWidget(); }
UInt32 CQtBenchmarkWindow::GetTickCount() const { return UInt32(clock_.elapsed()); }
void CQtBenchmarkWindow::NormalizeSize(bool fullNormalize) {
    QtNormalizeDialogSize(*this, fullNormalize);
}
void CQtBenchmarkWindow::NormalizePosition() { QtNormalizeDialogPosition(*this); }
void CQtBenchmarkWindow::SendMsg_NextDlgCtl_CtlId(unsigned id) {
    if (auto *view = GetItem(id))
        view->setFocus();
}
quintptr CQtBenchmarkWindow::SetTimer(quintptr id, unsigned milliseconds) {
    timer_.stop();
    disconnect(&timer_, nullptr, this, nullptr);
    connect(&timer_, &QTimer::timeout, this, [this, id] { OnTimer(id, 0); });
    timer_.start(int(milliseconds));
    return id;
}
void CQtBenchmarkWindow::KillTimer(quintptr) { timer_.stop(); }
bool CQtBenchmarkWindow::PostMsg(unsigned message, quintptr wparam) {
    // Original PostMsg_Finish succeeds without posting after Attach(NULL).
    if (!postsEnabled_)
        return true;
    return QMetaObject::invokeMethod(
        this, [this, message, wparam] { OnMessage(message, wparam, 0); }, Qt::QueuedConnection);
}
qintptr CQtBenchmarkWindow::Create(unsigned resource, QWidget *parent) {
    if (parent)
        setParent(parent, windowFlags());
    BuildControls(resource == IDD_BENCH_TOTAL);
    OnInit();
    if (resource == IDD_BENCH_TOTAL)
        GetItem(IDE_BENCH2_EDIT)->setFocus();
    else {
        const unsigned order[] = {IDB_RESTART,
                                  IDB_STOP,
                                  IDHELP,
                                  IDCANCEL,
                                  IDC_BENCH_DICTIONARY,
                                  IDC_BENCH_NUM_THREADS,
                                  IDC_BENCH_NUM_PASSES};
        for (unsigned i = 1; i < sizeof(order) / sizeof(order[0]); ++i)
            QWidget::setTabOrder(GetItem(order[i - 1]), GetItem(order[i]));
        GetItem(IDB_RESTART)->setFocus();
    }
    const int result = exec();
    OnDestroy();
    return result;
}
void CQtBenchmarkWindow::OnCancel() { QDialog::reject(); }
void CQtBenchmarkWindow::reject() { OnCancel(); }
void CQtBenchmarkWindow::closeEvent(QCloseEvent *event) {
    event->ignore();
    OnCancel();
}
void CQtBenchmarkWindow::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    OnSize(0, event->size().width(), event->size().height());
}
