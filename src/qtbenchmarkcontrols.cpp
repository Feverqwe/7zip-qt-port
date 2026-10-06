#include "qtbenchmarkcontrols.h"
#include "7zip/UI/GUI/BenchmarkDialogRes.h"
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
QPushButton *CQtBenchmarkWindow::button(unsigned id, const char *name, const QString &text) {
    auto *view = new QPushButton(text, this);
    Register(id, name, view);
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
        elapsed->addWidget(label(IDT_BENCH_ELAPSED_VAL, "IDT_BENCH_ELAPSED_VAL"));
        elapsed->addStretch();
        outer->addLayout(elapsed);
        auto *console = new QPlainTextEdit(this);
        console->setReadOnly(true);
        console->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        Register(IDE_BENCH2_EDIT, "IDE_BENCH2_EDIT", console);
        outer->addWidget(console, 1);
    } else {
        auto *body = new QHBoxLayout;
        BuildNormalControls(body);
        outer->addLayout(body, 1);
    }
    auto *footer = new QHBoxLayout;
    footer->addStretch();
    footer->addWidget(button(9, "IDHELP", QtLangText(9)));
    footer->addWidget(button(2, "IDCANCEL", QtLangText(2)));
    outer->addLayout(footer);
    resize(totalMode ? QSize(760, 560) : QSize(960, 650));
}
void CQtBenchmarkWindow::BuildNormalControls(QHBoxLayout *body) {
    auto *left = new QWidget(this);
    auto *grid = new QGridLayout(left);
    grid->addWidget(
        label(IDT_BENCH_DICTIONARY, "IDT_BENCH_DICTIONARY", QtLangText(IDT_BENCH_DICTIONARY)), 0,
        0);
    grid->addWidget(combo(IDC_BENCH_DICTIONARY, "IDC_BENCH_DICTIONARY"), 0, 1);
    grid->addWidget(label(IDT_BENCH_MEMORY, "IDT_BENCH_MEMORY", QtLangText(IDT_BENCH_MEMORY)), 0,
                    2);
    grid->addWidget(label(IDT_BENCH_MEMORY_VAL, "IDT_BENCH_MEMORY_VAL"), 1, 2);
    grid->addWidget(
        label(IDT_BENCH_NUM_THREADS, "IDT_BENCH_NUM_THREADS", QtLangText(IDT_BENCH_NUM_THREADS)), 2,
        0);
    grid->addWidget(combo(IDC_BENCH_NUM_THREADS, "IDC_BENCH_NUM_THREADS"), 2, 1);
    grid->addWidget(label(IDT_BENCH_HARDWARE_THREADS, "IDT_BENCH_HARDWARE_THREADS"), 2, 2);
    qobject_cast<QLabel *>(GetItem(IDT_BENCH_DICTIONARY))->setBuddy(GetItem(IDC_BENCH_DICTIONARY));
    qobject_cast<QLabel *>(GetItem(IDT_BENCH_NUM_THREADS))
        ->setBuddy(GetItem(IDC_BENCH_NUM_THREADS));
    grid->addLayout(BuildMetrics(), 3, 0, 1, 3);
    auto *error = label(IDT_BENCH_ERROR_MESSAGE, "IDT_BENCH_ERROR_MESSAGE");
    error->setWordWrap(true);
    grid->addWidget(error, 4, 0, 1, 3, Qt::AlignLeft);
    grid->addLayout(BuildSummary(), 5, 0, 1, 3);
    const unsigned info[] = {IDT_BENCH_CPU, IDT_BENCH_CPU_FEATURE, IDT_BENCH_VER, IDT_BENCH_SYS1,
                             IDT_BENCH_SYS2};
    const char *names[] = {"IDT_BENCH_CPU", "IDT_BENCH_CPU_FEATURE", "IDT_BENCH_VER",
                           "IDT_BENCH_SYS1", "IDT_BENCH_SYS2"};
    for (int i = 0; i < 5; ++i) {
        auto *view = label(info[i], names[i]);
        view->setWordWrap(true);
        grid->addWidget(view, 6 + i, 0, 1, 3, Qt::AlignLeft);
    }
    grid->setRowStretch(11, 1);
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("benchmarkControlsScroll");
    scroll->setWidgetResizable(true);
    scroll->setWidget(left);
    body->addWidget(scroll, 3);
    auto *right = new QVBoxLayout;
    right->addWidget(button(IDB_RESTART, "IDB_RESTART", QtLangText(IDB_RESTART)));
    right->addWidget(button(IDB_STOP, "IDB_STOP", QtLangText(IDB_STOP)));
    auto *log = new QPlainTextEdit(this);
    log->setReadOnly(true);
    log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    Register(IDT_BENCH_LOG, "IDT_BENCH_LOG", log);
    right->addWidget(log, 1);
    body->addLayout(right, 1);
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
    auto *metrics = new QGridLayout;
    for (int col = 0; col < 5; ++col)
        metrics->addWidget(label(captions[col], captionNames[col], QtLangText(captions[col])), 0,
                           col + 1);
    for (int group = 0; group < 2; ++group) {
        const unsigned id = group ? IDG_BENCH_DECOMPRESSING : IDG_BENCH_COMPRESSING;
        metrics->addWidget(
            label(id, group ? "IDG_BENCH_DECOMPRESSING" : "IDG_BENCH_COMPRESSING", QtLangText(id)),
            1 + group * 3, 0, 1, 6, Qt::AlignLeft);
        for (int row = 0; row < 2; ++row) {
            const unsigned textId = row ? IDT_BENCH_RESULTING : IDT_BENCH_CURRENT;
            const unsigned fieldId = group ? textId + 50 : textId;
            metrics->addWidget(label(fieldId,
                                     group ? (row ? "IDT_BENCH_RESULTING2" : "IDT_BENCH_CURRENT2")
                                           : (row ? "IDT_BENCH_RESULTING" : "IDT_BENCH_CURRENT"),
                                     QtLangText(textId)),
                               2 + group * 3 + row, 0);
            for (int col = 0; col < 5; ++col) {
                const unsigned valueId = values[group][row][col];
                const QByteArray name = "benchmarkValue" + QByteArray::number(valueId);
                auto *view = label(valueId, name.constData());
                view->setMinimumWidth(62);
                metrics->addWidget(view, 2 + group * 3 + row, col + 1);
            }
        }
    }
    return metrics;
}
QGridLayout *CQtBenchmarkWindow::BuildSummary() {
    auto *summary = new QGridLayout;
    summary->addWidget(label(IDT_BENCH_ELAPSED, "IDT_BENCH_ELAPSED", QtLangText(IDT_BENCH_ELAPSED)),
                       0, 0);
    summary->addWidget(label(IDT_BENCH_ELAPSED_VAL, "IDT_BENCH_ELAPSED_VAL"), 0, 1);
    summary->addWidget(label(IDT_BENCH_PASSES, "IDT_BENCH_PASSES", QtLangText(IDT_BENCH_PASSES)), 1,
                       0);
    summary->addWidget(label(IDT_BENCH_PASSES_VAL, "IDT_BENCH_PASSES_VAL"), 1, 1);
    summary->addWidget(combo(IDC_BENCH_NUM_PASSES, "IDC_BENCH_NUM_PASSES"), 2, 0);
    auto *total = new QGroupBox(QtLangText(IDG_BENCH_TOTAL_RATING), this);
    Register(IDG_BENCH_TOTAL_RATING, "IDG_BENCH_TOTAL_RATING", total);
    auto *totalLayout = new QHBoxLayout(total);
    totalLayout->addWidget(label(IDT_BENCH_TOTAL_USAGE_VAL, "IDT_BENCH_TOTAL_USAGE_VAL"));
    totalLayout->addWidget(label(IDT_BENCH_TOTAL_RPU_VAL, "IDT_BENCH_TOTAL_RPU_VAL"));
    totalLayout->addWidget(label(IDT_BENCH_TOTAL_RATING_VAL, "IDT_BENCH_TOTAL_RATING_VAL"));
    summary->addWidget(total, 0, 2, 3, 1);
    return summary;
}
void CQtBenchmarkWindow::SetItemText(unsigned id, const UString &text) {
    const QString value = QString::fromWCharArray(text.Ptr());
    if (auto *label = qobject_cast<QLabel *>(GetItem(id))) {
        label->setText(value);
        // A left-aligned spanning item has its natural width, whereas QGridLayout
        // calculates its row height for the entire span. Retain the wrapped
        // label's own required height after the original source updates text.
        if (label->wordWrap())
            label->setMinimumHeight(label->heightForWidth(label->sizeHint().width()));
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
    if (!postsEnabled_)
        return false;
    return QMetaObject::invokeMethod(
        this, [this, message, wparam] { OnMessage(message, wparam, 0); }, Qt::QueuedConnection);
}
qintptr CQtBenchmarkWindow::Create(unsigned resource, QWidget *parent) {
    if (parent)
        setParent(parent, windowFlags());
    BuildControls(resource == IDD_BENCH_TOTAL);
    OnInit();
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
