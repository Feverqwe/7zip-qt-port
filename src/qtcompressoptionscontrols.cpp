#include "qtcompressoptionscontrols.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
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
    resize(QtDialogUnits(font()).pixels(QSize(256, 248)));
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
        if (id == IDT_COMPRESS_TIME_PREC)
            widget->setProperty("languageWrapWidth", 160);
        if (id >= IDX_COMPRESS_NT_SYM_LINKS && id <= IDX_COMPRESS_PRESERVE_ATIME)
            QtBindLangText(widget, id);
    };
    const auto checkbox = [&](unsigned id, const char *name, const QString &text, QWidget *parent) {
        auto *box = new QCheckBox(text.isEmpty() ? QStringLiteral(":") : text, parent);
        if (text.isEmpty())
            box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        registerControl(id, name, box);
        if (!text.isEmpty())
            QtBindResourceText(box, id, text);
        connect(box, &QCheckBox::clicked, this, [this, id, box] { OnButtonClicked(id, box); });
        return box;
    };
    // The fixed source template reserves this area even when NTFS is hidden.
    // Its empty space can contract in the accessible compact Qt view.
    auto *ntfsSlot = new QWidget(this);
    auto *slot = new QVBoxLayout(ntfsSlot);
    slot->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_MAC
    // QMacStyle's group backing rectangle extends one logical pixel below
    // its layout item; retain that pixel inside the slot's exposed area.
    slot->setContentsMargins(0, 0, 0, 1);
#endif
    slot->setSpacing(0);
    auto *ntfs = new QGroupBox("NTFS", ntfsSlot);
    registerControl(IDG_COMPRESS_NTFS, "IDG_COMPRESS_NTFS", ntfs);
    auto *ntfsLayout = new QVBoxLayout(ntfs);
    ntfsLayout->setSpacing(3);
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_SYM_LINKS, "IDX_COMPRESS_NT_SYM_LINKS",
                                   "Store symbolic links", ntfs));
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_HARD_LINKS, "IDX_COMPRESS_NT_HARD_LINKS",
                                   "Store hard links", ntfs));
    ntfsLayout->addWidget(checkbox(IDX_COMPRESS_NT_ALT_STREAMS, "IDX_COMPRESS_NT_ALT_STREAMS",
                                   "Store alternate data streams", ntfs));
    ntfsLayout->addWidget(
        checkbox(IDX_COMPRESS_NT_SECUR, "IDX_COMPRESS_NT_SECUR", "Store file security", ntfs));
    slot->addWidget(ntfs);
    slot->addStretch();
    layout->addWidget(ntfsSlot, 1);
    auto *info = new QLabel(this);
    registerControl(IDT_COMPRESS_TIME_INFO, "IDT_COMPRESS_TIME_INFO", info);
    info->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    layout->addWidget(info);
    auto *time = new QGroupBox("Time", this);
    time->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    registerControl(IDG_COMPRESS_TIME, "IDG_COMPRESS_TIME", time);
    QtBindResourceText(time, IDG_COMPRESS_TIME, "Time");
    auto *grid = new QGridLayout(time);
    grid->setVerticalSpacing(3);
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
    ntfs->setMinimumHeight(qMax(y(72), ntfs->minimumSizeHint().height()));
    auto *time = static_cast<QGroupBox *>(GetItem(IDG_COMPRESS_TIME));
    auto *grid = static_cast<QGridLayout *>(time->layout());
    grid->setContentsMargins(x(8), y(12), x(8), y(4));
    grid->setSpacing(0);
#ifdef Q_OS_MAC
    // Cocoa's combo backing extends left of its layout item. Native spacing
    // keeps it separate from the preceding precision label.
    grid->setHorizontalSpacing(
        qMax(0, style()->layoutSpacing(QSizePolicy::Label, QSizePolicy::ComboBox, Qt::Horizontal)));
#endif
    grid->setColumnMinimumWidth(0, x(18));
    grid->setColumnStretch(1, 1);
    for (int row = 0; row < 6; ++row)
        grid->setRowMinimumHeight(row, y(row < 4 ? 14 : 20));
    auto *precision = static_cast<QComboBox *>(GetItem(IDC_COMPRESS_TIME_PREC));
    precision->setFixedWidth(qMax(x(76), precision->minimumSizeHint().width()));
    auto *buttons = findChild<QDialogButtonBox *>("compressOptionsButtons");
    for (auto *button : buttons->buttons()) {
        button->setMinimumWidth(qMax(x(64), button->sizeHint().width()));
        button->setMinimumHeight(qMax(y(16), button->sizeHint().height()));
    }
}

void CQtCompressOptionsDialog::setVisible(bool visible) {
    // Apply source capability visibility before Qt computes the initial
    // minimum size, so a hidden NTFS group does not enlarge compact dialogs.
    const bool initialize = visible && !initialized_;
    if (initialize) {
        initialized_ = true;
        OnInit();
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
    if (id == IDG_COMPRESS_NTFS) {
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
}
void CQtCompressOptionsDialog::EnableItem(unsigned id, bool enabled) {
    GetItem(id)->setEnabled(enabled);
}
void CQtCompressOptionsDialog::SetItemText(unsigned id, const UString &text) {
    static_cast<QLabel *>(GetItem(id))->setText(QString::fromWCharArray(text.Ptr()));
}
void CQtCompressOptionsDialog::NormalizePosition() { QtNormalizeDialogPosition(*this); }
bool CQtCompressOptionsDialog::OnButtonClicked(unsigned, QWidget *) { return false; }
