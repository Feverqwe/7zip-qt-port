#include "qtcompressoptionscontrols.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "qtlangcontrols.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

void CQtCompressCombo::Attach(QWidget *widget) { view_ = static_cast<QComboBox *>(widget); }
int CQtCompressCombo::GetCount() const { return view_->count(); }
quint32 CQtCompressCombo::GetItemData_of_CurSel() const { return view_->currentData().toUInt(); }
quint32 CQtCompressCombo::GetItemData(int index) const { return view_->itemData(index).toUInt(); }
void CQtCompressCombo::SetItemText(int index, const UString &text) {
    view_->setItemText(index, QString::fromWCharArray(text));
}
int CQtCompressCombo::AddString_SetItemData(const UString &text, qintptr value) {
    view_->addItem(QString::fromWCharArray(text.Ptr()), quint32(value));
    return view_->count() - 1;
}
void CQtCompressCombo::ResetContent() { view_->clear(); }
void CQtCompressCombo::SetCurSel(int index) { view_->setCurrentIndex(index); }

CQtCompressOptionsDialog::CQtCompressOptionsDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("compressOptionsDialog");
    QtBindLangText(this, IDB_COMPRESS_OPTIONS, "Options");
    buildControls();
    resize(520, 400);
}

void CQtCompressOptionsDialog::buildControls() {
    auto *layout = new QVBoxLayout(this);
    const auto registerControl = [this](unsigned id, const char *name, QWidget *widget) {
        controls_.insert(id, widget);
        widget->setObjectName(name);
        if (id == IDX_COMPRESS_ZTIME)
            widget->setProperty("languageWrapWidth", 300);
        if (id >= IDX_COMPRESS_NT_SYM_LINKS && id <= IDX_COMPRESS_PRESERVE_ATIME)
            QtBindLangText(widget, id);
    };
    const auto checkbox = [&](unsigned id, const char *name, const QString &text, QWidget *parent) {
        auto *box = new QCheckBox(text, parent);
        if (text.isEmpty())
            box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        registerControl(id, name, box);
        if (!text.isEmpty())
            QtBindResourceText(box, id, text);
        connect(box, &QCheckBox::clicked, this, [this, id, box] { OnButtonClicked(id, box); });
        return box;
    };
    auto *ntfs = new QGroupBox("NTFS", this);
    registerControl(IDG_COMPRESS_NTFS, "IDG_COMPRESS_NTFS", ntfs);
    auto *ntfsLayout = new QVBoxLayout(ntfs);
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_SYM_LINKS, "IDX_COMPRESS_NT_SYM_LINKS",
                                   "Store symbolic links", ntfs));
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_HARD_LINKS, "IDX_COMPRESS_NT_HARD_LINKS",
                                   "Store hard links", ntfs));
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_ALT_STREAMS, "IDX_COMPRESS_NT_ALT_STREAMS",
                                   "Store alternate data streams", ntfs));
    ntfsLayout->addWidget(
        checkbox(IDX_COMPRESS_NT_SECUR, "IDX_COMPRESS_NT_SECUR", "Store file security", ntfs));
    layout->addWidget(ntfs);
    auto *info = new QLabel(this);
    registerControl(IDT_COMPRESS_TIME_INFO, "IDT_COMPRESS_TIME_INFO", info);
    layout->addWidget(info);
    auto *time = new QGroupBox("Time", this);
    registerControl(IDG_COMPRESS_TIME, "IDG_COMPRESS_TIME", time);
    QtBindResourceText(time, IDG_COMPRESS_TIME, "Time");
    auto *grid = new QGridLayout(time);
    grid->addWidget(checkbox(IDX_COMPRESS_PREC_SET, "IDX_COMPRESS_PREC_SET", {}, time), 0, 0);
    QtBindLocalText(GetItem(IDX_COMPRESS_PREC_SET), "Set time precision", "Задать точность времени",
                    true);
    auto *precisionLabel = new QLabel("Timestamp precision:", time);
    registerControl(IDT_COMPRESS_TIME_PREC, "IDT_COMPRESS_TIME_PREC", precisionLabel);
    QtBindResourceText(precisionLabel, IDT_COMPRESS_TIME_PREC, "Timestamp precision:");
    grid->addWidget(precisionLabel, 0, 1);
    auto *precision = new QComboBox(time);
    precision->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    precision->setMinimumContentsLength(10);
    registerControl(IDC_COMPRESS_TIME_PREC, "IDC_COMPRESS_TIME_PREC", precision);
    precisionLabel->setBuddy(precision);
    grid->addWidget(precision, 0, 2);
    connect(precision, &QComboBox::activated, this,
            [this](int) { OnCommand(CBN_SELCHANGE, IDC_COMPRESS_TIME_PREC, 0); });
    struct CTimeRow {
        unsigned set, value;
        const char *setName, *valueName, *text;
    };
    const CTimeRow rows[] = {{IDX_COMPRESS_MTIME_SET, IDX_COMPRESS_MTIME, "IDX_COMPRESS_MTIME_SET",
                              "IDX_COMPRESS_MTIME", "Store modification time"},
                             {IDX_COMPRESS_CTIME_SET, IDX_COMPRESS_CTIME, "IDX_COMPRESS_CTIME_SET",
                              "IDX_COMPRESS_CTIME", "Store creation time"},
                             {IDX_COMPRESS_ATIME_SET, IDX_COMPRESS_ATIME, "IDX_COMPRESS_ATIME_SET",
                              "IDX_COMPRESS_ATIME", "Store last access time"},
                             {IDX_COMPRESS_ZTIME_SET, IDX_COMPRESS_ZTIME, "IDX_COMPRESS_ZTIME_SET",
                              "IDX_COMPRESS_ZTIME", "Set archive time to latest file time"}};
    int row = 1;
    for (const auto &item : rows) {
        auto *set = checkbox(item.set, item.setName, {}, time);
        set->setProperty("languageToolTip", true);
        QtBindLangText(set, item.value, {}, true);
        grid->addWidget(set, row, 0);
        grid->addWidget(checkbox(item.value, item.valueName, item.text, time), row, 1, 1, 2);
        ++row;
    }
    auto *preserve = checkbox(IDX_COMPRESS_PRESERVE_ATIME, "IDX_COMPRESS_PRESERVE_ATIME",
                              "Do not change source files last access time", time);
#ifndef Q_OS_WIN
    preserve->setEnabled(false);
    QtBindLocalToolTip(preserve, "The official engine supports this option only on Windows.",
                       "Официальный движок поддерживает эту опцию только в Windows.");
#endif
    grid->addWidget(preserve, row, 0, 1, 3);
    layout->addWidget(time);
    layout->addStretch();
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    buttons->setObjectName("compressOptionsButtons");
    buttons->button(QDialogButtonBox::Help)->setEnabled(false);
    connect(buttons, &QDialogButtonBox::accepted, this, &CQtCompressOptionsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void CQtCompressOptionsDialog::setVisible(bool visible) {
    // Apply source capability visibility before Qt computes the initial
    // minimum size, so a hidden NTFS group does not enlarge compact dialogs.
    if (visible && !initialized_) {
        initialized_ = true;
        OnInit();
    }
    QDialog::setVisible(visible);
}
void CQtCompressOptionsDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && initialized_)
        OnLanguageChange();
    QDialog::changeEvent(event);
}
QWidget *CQtCompressOptionsDialog::GetItem(unsigned id) const { return controls_.value(id); }
void CQtCompressOptionsDialog::CheckButton(unsigned id, bool value) {
    static_cast<QCheckBox *>(GetItem(id))->setChecked(value);
}
bool CQtCompressOptionsDialog::IsButtonCheckedBool(unsigned id) const {
    return static_cast<QCheckBox *>(GetItem(id))->isChecked();
}
void CQtCompressOptionsDialog::ShowItem_Bool(unsigned id, bool visible) {
    GetItem(id)->setVisible(visible);
}
void CQtCompressOptionsDialog::EnableItem(unsigned id, bool enabled) {
    GetItem(id)->setEnabled(enabled);
}
void CQtCompressOptionsDialog::SetItemText(unsigned id, const UString &text) {
    static_cast<QLabel *>(GetItem(id))->setText(QString::fromWCharArray(text.Ptr()));
}
void CQtCompressOptionsDialog::NormalizePosition() {
    // Qt supplies dialog placement; clamp only if that placement lies outside
    // the current screen, matching the native platform-service responsibility.
    if (screen() && !screen()->availableGeometry().intersects(frameGeometry()))
        move(screen()->availableGeometry().center() - rect().center());
}
bool CQtCompressOptionsDialog::OnButtonClicked(unsigned, QWidget *) { return false; }
