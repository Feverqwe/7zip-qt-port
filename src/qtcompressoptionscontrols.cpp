#include "qtcompressoptionscontrols.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
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
#include <QShortcut>
#include <QStyle>
#include <QVBoxLayout>

void CQtCompressCombo::Attach(QWidget *widget) { view_ = static_cast<QComboBox *>(widget); }
int CQtCompressCombo::GetCount() const { return view_->count(); }
quint32 CQtCompressCombo::GetItemData_of_CurSel() const {
    return GetItemData(view_->currentIndex());
}
quint32 CQtCompressCombo::GetItemData(int index) const {
    // Native CB_GETITEMDATA returns CB_ERR for an absent/out-of-range item.
    if (index < 0 || index >= view_->count())
        return quint32(-1);
    return view_->itemData(index).toUInt();
}
void CQtCompressCombo::SetItemText(int index, const UString &text) {
    view_->setItemText(index, QString::fromWCharArray(text));
}
int CQtCompressCombo::AddString_SetItemData(const UString &text, qintptr value) {
    const bool hadSelection = view_->currentIndex() >= 0;
    view_->addItem(QString::fromWCharArray(text.Ptr()), quint32(value));
    // Native AddString does not select the first item. SetPrec owns selection.
    if (!hadSelection)
        view_->setCurrentIndex(-1);
    return view_->count() - 1;
}
void CQtCompressCombo::ResetContent() { view_->clear(); }
void CQtCompressCombo::SetCurSel(int index) { view_->setCurrentIndex(index); }

CQtCompressOptionsDialog::CQtCompressOptionsDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("compressOptionsDialog");
    QtBindLangText(this, IDB_COMPRESS_OPTIONS, "Options");
    buildControls();
    applySourceGeometry();
}

void CQtCompressOptionsDialog::buildControls() {
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(3);
    const auto registerControl = [this](unsigned id, const char *name, QWidget *widget) {
        controls_.insert(id, widget);
        widget->setObjectName(name);
        if (id == IDX_COMPRESS_ZTIME || id == IDX_COMPRESS_PRESERVE_ATIME)
            widget->setProperty("languageWrapWidth", 300);
        if (id == IDX_COMPRESS_MTIME || id == IDX_COMPRESS_CTIME || id == IDX_COMPRESS_ATIME)
            widget->setProperty("languageWrapWidth", 300);
        if (id >= IDX_COMPRESS_NT_SYM_LINKS && id <= IDX_COMPRESS_PRESERVE_ATIME)
            QtBindLangText(widget, id);
    };
    const auto checkbox = [&](unsigned id, const char *name, const QString &text, QWidget *parent) {
        auto *box = new QCheckBox(text.isEmpty() ? QStringLiteral(":") : text, parent);
        if (text.isEmpty())
            box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        registerControl(id, name, box);
        if (text.isEmpty()) {
            box->setText(QCoreApplication::translate("qtcompressoptionscontrols", "Set"));
            QtBindLocalToolTip(box, "qtcompressoptionscontrols",
                               QT_TRANSLATE_NOOP("qtcompressoptionscontrols",
                                                 "Override the format default for this option."));
        }
        if (!text.isEmpty())
            QtBindResourceText(box, id, text);
        connect(box, &QCheckBox::clicked, this, [this, id, box] { OnButtonClicked(id, box); });
        return box;
    };
    auto *ntfsSlot = new QWidget(this);
    ntfsSlot->setObjectName("compressOptionsNtfsSlot");
    auto *slot = new QVBoxLayout(ntfsSlot);
    slot->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_MACOS
    // QMacStyle's group backing rectangle extends one logical pixel below
    // its layout item; retain that pixel inside the slot's exposed area.
    slot->setContentsMargins(0, 0, 0, 1);
#endif
    slot->setSpacing(0);
    auto *ntfs = new QGroupBox("NTFS", ntfsSlot);
    registerControl(IDG_COMPRESS_NTFS, "IDG_COMPRESS_NTFS", ntfs);
    auto *ntfsLayout = new QGridLayout(ntfs);
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_SYM_LINKS, "IDX_COMPRESS_NT_SYM_LINKS",
                                   "Store symbolic links", ntfs),
                          0, 0);
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_HARD_LINKS, "IDX_COMPRESS_NT_HARD_LINKS",
                                   "Store hard links", ntfs),
                          1, 0);
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_ALT_STREAMS, "IDX_COMPRESS_NT_ALT_STREAMS",
                                   "Store alternate data streams", ntfs),
                          2, 0);
    ntfsLayout->addWidget(
        checkbox(IDX_COMPRESS_NT_SECUR, "IDX_COMPRESS_NT_SECUR", "Store file security", ntfs), 3,
        0);
    ntfsLayout->setRowStretch(4, 1);
    slot->addWidget(ntfs);
    slot->addStretch();
    layout->addWidget(ntfsSlot);
    auto *info = new QLabel(this);
    registerControl(IDT_COMPRESS_TIME_INFO, "IDT_COMPRESS_TIME_INFO", info);
    info->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    info->setContentsMargins(0, 0, 0, QtDialogUnits(font()).pixels(QSize(0, 4)).height());
    layout->addWidget(info);
    auto *time = new QGroupBox("Time", this);
    time->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    registerControl(IDG_COMPRESS_TIME, "IDG_COMPRESS_TIME", time);
    QtBindResourceText(time, IDG_COMPRESS_TIME, "Time");
    auto *grid = new QGridLayout(time);
    grid->setVerticalSpacing(3);
    grid->addWidget(checkbox(IDX_COMPRESS_PREC_SET, "IDX_COMPRESS_PREC_SET", {}, time), 0, 0);
    QtBindLocalText(GetItem(IDX_COMPRESS_PREC_SET), "qtcompressoptionscontrols",
                    QT_TRANSLATE_NOOP("qtcompressoptionscontrols", "Set time precision"), true);
    auto *precisionLabel = new QLabel("Timestamp precision:", time);
    precisionLabel->setWordWrap(true);
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
        QtBindLangText(set, item.value, {}, true);
        grid->addWidget(set, row, 0);
        grid->addWidget(checkbox(item.value, item.valueName, item.text, time), row, 1, 1, 2);
        ++row;
    }
    auto *preserve = checkbox(IDX_COMPRESS_PRESERVE_ATIME, "IDX_COMPRESS_PRESERVE_ATIME",
                              "Do not change source files last access time", time);
#ifndef Q_OS_WIN
    preserve->setEnabled(false);
    preserve->hide();
    QtBindLocalToolTip(
        preserve, "qtcompressoptionscontrols",
        QT_TRANSLATE_NOOP("qtcompressoptionscontrols",
                          "The official engine supports this option only on Windows."));
#endif
    grid->addWidget(preserve, row, 0, 1, 3);
    layout->addWidget(time);
    layout->addStretch();
    layout->addSpacing(QtDialogUnits(font()).pixels(QSize(0, 6)).height());
    auto *buttons = new QtLangButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    buttons->setObjectName("compressOptionsButtons");
    connect(buttons, &QDialogButtonBox::helpRequested, this, [this] { OnHelp(); });
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, [this] { OnHelp(); });
    connect(buttons, &QDialogButtonBox::accepted, this, &CQtCompressOptionsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void CQtCompressOptionsDialog::applySourceGeometry() {
    const QtDialogUnits units(font());
    const auto x = [&](int value) { return units.pixels(QSize(value, 0)).width(); };
    const auto y = [&](int value) { return units.pixels(QSize(0, value)).height(); };
    auto *root = static_cast<QVBoxLayout *>(layout());
    root->setContentsMargins(x(8), y(8), x(8), y(8));
    root->setSpacing(y(6));
    auto *ntfs = static_cast<QGroupBox *>(GetItem(IDG_COMPRESS_NTFS));
    ntfs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ntfs->layout()->setContentsMargins(x(8), y(12), x(8), y(4));
    ntfs->layout()->setSpacing(0);
    auto *ntfsGrid = static_cast<QGridLayout *>(ntfs->layout());
    for (int row = 0; row < 4; ++row)
        ntfsGrid->setRowMinimumHeight(row, y(14));
    auto *time = static_cast<QGroupBox *>(GetItem(IDG_COMPRESS_TIME));
    auto *grid = static_cast<QGridLayout *>(time->layout());
    grid->setContentsMargins(x(8), y(6), x(8), y(4));
    grid->setSpacing(0);
    grid->setHorizontalSpacing(x(6));
#ifdef Q_OS_MACOS
    // Cocoa's combo backing extends left of its layout item. Native spacing
    // keeps it separate from the preceding precision label.
    grid->setHorizontalSpacing(qMax(
        x(6), style()->layoutSpacing(QSizePolicy::Label, QSizePolicy::ComboBox, Qt::Horizontal)));
#endif
    grid->setColumnMinimumWidth(0, x(18));
    grid->setColumnStretch(1, 1);
    auto *precision = static_cast<QComboBox *>(GetItem(IDC_COMPRESS_TIME_PREC));
    precision->setFixedWidth(qMax(x(76), precision->minimumSizeHint().width()));
    auto *buttons = findChild<QDialogButtonBox *>("compressOptionsButtons");
    for (auto *button : buttons->buttons()) {
        button->setMinimumSize(button->sizeHint());
    }
}

void CQtCompressOptionsDialog::setVisible(bool visible) {
    // Apply source capability visibility before Qt computes the initial
    // minimum size, so a hidden NTFS group does not enlarge compact dialogs.
    const bool initialize = visible && !initialized_;
    if (initialize) {
        initialized_ = true;
        OnInit();
        // Size after applying format/platform visibility, without reserving
        // space for groups that the user cannot use.
        if (!testAttribute(Qt::WA_Resized))
            adjustSize();
        NormalizePosition();
    }
    QDialog::setVisible(visible);
    if (initialize)
        focusControl(this, true, Qt::OtherFocusReason);
}

bool CQtCompressOptionsDialog::focusControl(QWidget *start, bool next, Qt::FocusReason reason) {
    // Win32's dialog manager traverses visible, enabled WS_TABSTOP controls.
    // Keep that source contract even when the native Qt style prefers only
    // text inputs for keyboard traversal. Qt still owns the actual tab chain.
    QWidget *control = start;
    do {
        control = next ? control->nextInFocusChain() : control->previousInFocusChain();
        if (isAncestorOf(control) && control->isVisibleTo(this) && control->isEnabled() &&
            (control->focusPolicy() & Qt::TabFocus)) {
            control->setFocus(reason);
            return true;
        }
    } while (control != start);
    return false;
}

bool CQtCompressOptionsDialog::focusNextPrevChild(bool next) {
    return focusControl(focusWidget() ? focusWidget() : this, next,
                        next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
}
void CQtCompressOptionsDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && initialized_) {
        OnLanguageChange();
        // Resource bindings retain the descriptive accessible option names.
        // The visible override caption is shared by these source controls.
        for (unsigned id : {IDX_COMPRESS_PREC_SET, IDX_COMPRESS_MTIME_SET, IDX_COMPRESS_CTIME_SET,
                            IDX_COMPRESS_ATIME_SET, IDX_COMPRESS_ZTIME_SET})
            static_cast<QCheckBox *>(GetItem(id))
                ->setText(QCoreApplication::translate("qtcompressoptionscontrols", "Set"));
    }
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
    const unsigned ntfsIds[] = {IDX_COMPRESS_NT_SYM_LINKS, IDX_COMPRESS_NT_HARD_LINKS,
                                IDX_COMPRESS_NT_ALT_STREAMS, IDX_COMPRESS_NT_SECUR};
    for (int row = 0; row < 4; ++row)
        if (id == ntfsIds[row]) {
            auto *grid = static_cast<QGridLayout *>(GetItem(IDG_COMPRESS_NTFS)->layout());
            grid->setRowMinimumHeight(
                row, visible ? QtDialogUnits(font()).pixels(QSize(0, 14)).height() : 0);
        }
    if (id == IDG_COMPRESS_NTFS) {
        GetItem(id)->parentWidget()->setVisible(visible);
        // Source NTFS metadata views have a wider compact contract (480px).
        // Keep their original one-line mtime/ctime captions; the narrower ZIP
        // time-only view needs wrapping to fit its 440px contract on Windows.
        for (const unsigned timeId : {IDX_COMPRESS_MTIME, IDX_COMPRESS_CTIME, IDX_COMPRESS_ATIME}) {
            auto *control = GetItem(timeId);
            control->setProperty("languageWrapWidth",
                                 visible ? (timeId == IDX_COMPRESS_ATIME ? 380 : 0) : 300);
            QtReloadLangTexts(*control);
        }
    }
    const unsigned timeIds[] = {IDX_COMPRESS_MTIME, IDX_COMPRESS_CTIME, IDX_COMPRESS_ATIME,
                                IDX_COMPRESS_ZTIME};
    for (int row = 0; row < 4; ++row)
        if (id == timeIds[row]) {
            auto *grid = static_cast<QGridLayout *>(GetItem(IDG_COMPRESS_TIME)->layout());
            grid->setRowMinimumHeight(
                row + 1,
                visible ? QtDialogUnits(font()).pixels(QSize(0, row < 3 ? 14 : 20)).height() : 0);
        }
}
void CQtCompressOptionsDialog::EnableItem(unsigned id, bool enabled) {
    GetItem(id)->setEnabled(enabled);
}
void CQtCompressOptionsDialog::SetItemText(unsigned id, const UString &text) {
    QString value = QString::fromWCharArray(text.Ptr());
    if (id == IDT_COMPRESS_TIME_INFO && value.contains(':'))
        value = QtLangText(IDT_COMPRESS_FORMAT, "Archive format:").remove('&') +
                value.mid(value.indexOf(':') + 1);
    static_cast<QLabel *>(GetItem(id))->setText(value);
}
void CQtCompressOptionsDialog::NormalizePosition() { QtNormalizeDialogPosition(*this); }
bool CQtCompressOptionsDialog::OnButtonClicked(unsigned, QWidget *) { return false; }
