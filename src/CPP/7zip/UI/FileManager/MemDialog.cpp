// Adapted from 7-Zip MemDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "MemDialog.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "Common/StringToInt.h"
#include "LangUtils.h"
#include "Windows/ErrorMsg.h"
#include "Windows/System.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QAbstractButton>
#include <QCheckBox>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

void CMemDialog::BuildPresentation() {
    setObjectName("memDialog");
    QtBindResourceText(this, IDD_MEM, "Memory usage request");
    resize(680, 460);
    auto *layout = new QVBoxLayout(this);
    auto *message = new QLabel(this);
    message->setObjectName("memMessage");
    message->setTextFormat(Qt::PlainText);
    message->setWordWrap(true);
    _controls.insert(IDT_MEM_MESSAGE, message);
    layout->addWidget(message);
    auto *save = new QCheckBox(this);
    QtBindResourceText(save, IDX_MEM_SAVE_LIMIT, "Change allowed limit for next operations");
    save->setObjectName("memSaveLimit");
    _controls.insert(IDX_MEM_SAVE_LIMIT, save);
    layout->addWidget(save);
    connect(save, &QCheckBox::toggled, this, &CMemDialog::EnableSpin);
    auto *row = new QHBoxLayout;
    auto *edit = new QLineEdit(this);
    edit->setObjectName("memLimit");
    QtBindLangText(edit, IDS_MEM_CURRENT_MEM_LIMIT, "Current allowed limit", true);
    edit->setMaximumWidth(130);
    _controls.insert(IDE_MEM_SPIN_EDIT, edit);
    row->addWidget(edit);
    auto *spin = new QWidget(this);
    spin->setObjectName("memSpin");
    auto *arrows = new QVBoxLayout(spin);
    arrows->setContentsMargins(0, 0, 0, 0);
    arrows->setSpacing(0);
    for (bool up : {true, false}) {
        auto *button = new QToolButton(spin);
        button->setArrowType(up ? Qt::UpArrow : Qt::DownArrow);
        button->setObjectName(up ? "memSpinUp" : "memSpinDown");
        button->setAutoRepeat(true);
        arrows->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, up] { StepSpin(up); });
    }
    _controls.insert(IDC_MEM_SPIN, spin);
    row->addWidget(spin);
    auto *gb = new QLabel(this);
    gb->setObjectName("memGB");
    _controls.insert(IDT_MEM_GB, gb);
    row->addWidget(gb);
    row->addStretch();
    layout->addLayout(row);
    auto *group = new QGroupBox(this);
    QtBindResourceText(group, IDG_MEM_ACTION, "Action");
    group->setObjectName("memAction");
    auto *actions = new QVBoxLayout(group);
    auto *allow = new QRadioButton(group);
    auto *skip = new QRadioButton(group);
    QtBindResourceText(allow, IDR_MEM_ACTION_ALLOW, "&Allow archive unpacking");
    QtBindResourceText(skip, IDR_MEM_ACTION_SKIP_ARC, "&Skip archive unpacking");
    allow->setObjectName("memAllow");
    skip->setObjectName("memSkip");
    _controls.insert(IDR_MEM_ACTION_ALLOW, allow);
    _controls.insert(IDR_MEM_ACTION_SKIP_ARC, skip);
    actions->addWidget(allow);
    actions->addWidget(skip);
    auto *remember = new QCheckBox(group);
    QtBindResourceText(remember, IDX_MEM_REMEMBER, "&Repeat selected action for current operation");
    remember->setObjectName("memRemember");
    _controls.insert(IDX_MEM_REMEMBER, remember);
    actions->addWidget(remember);
    layout->addWidget(group);
    layout->addStretch();
    auto *buttons =
        new QtLangButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this, IDCONTINUE);
    buttons->setObjectName("memButtons");
    QtBindResourceText(buttons->button(QDialogButtonBox::Ok), 411, "Continue");
    connect(buttons, &QDialogButtonBox::accepted, this, &CMemDialog::OnContinue);
    connect(buttons, &QDialogButtonBox::rejected, this, &CMemDialog::reject);
    layout->addWidget(buttons);
}

void CMemDialog::EnableSpin(bool enable) {
    _controls.value(IDC_MEM_SPIN)->setEnabled(enable);
    _controls.value(IDE_MEM_SPIN_EDIT)->setEnabled(enable);
}

void CMemDialog::SetItemText(unsigned id, const UString &text) {
    const auto value = QString::fromWCharArray(text);
    if (auto *label = qobject_cast<QLabel *>(_controls.value(id)))
        label->setText(value);
    else
        static_cast<QLineEdit *>(_controls.value(id))->setText(value);
}

void CMemDialog::GetItemText(unsigned id, UString &text) const {
    text = static_cast<QLineEdit *>(_controls.value(id))->text().toStdWString().c_str();
}

bool CMemDialog::IsButtonCheckedBool(unsigned id) const {
    return static_cast<QAbstractButton *>(_controls.value(id))->isChecked();
}

void CMemDialog::StepSpin(bool up) {
    UString text;
    GetItemText(IDE_MEM_SPIN_EDIT, text);
    const wchar_t *end;
    UInt32 value = ConvertStringToUInt32(text, &end);
    if (*end || text.IsEmpty())
        value = _spinMin;
    else if (up && value < _spinMax)
        ++value;
    else if (!up && value > _spinMin)
        --value;
    value = qBound(_spinMin, value, _spinMax);
    text.Empty();
    text.Add_UInt32(value);
    SetItemText(IDE_MEM_SPIN_EDIT, text);
}

INT_PTR CMemDialog::Create(QWidget *parent) {
    if (parent && parent != parentWidget())
        setParent(parent, windowFlags());
    OnInit();
    return exec();
}

void CMemDialog::reject() { done(IDCANCEL); }

static void AddSize_GB(UString &s, UInt32 size_GB, UInt32 id) {
    s.Add_LF();
    s += "    ";
    s.Add_UInt32(size_GB);
    s += " GB : ";
    AddLangString(s, id);
}

void CMemDialog::AddInfoMessage_To_String(UString &s, const UInt32 *ramSize_GB) {
    AddLangString(s, IDS_MEM_REQUIRES_BIG_MEM);
    AddSize_GB(s, Required_GB, IDS_MEM_REQUIRED_MEM_SIZE);
    AddSize_GB(s, Limit_GB, IDS_MEM_CURRENT_MEM_LIMIT);
    if (ramSize_GB)
        AddSize_GB(s, *ramSize_GB, IDS_MEM_RAM_SIZE);
    if (!FilePath.IsEmpty()) {
        s.Add_LF();
        s += "File: ";
        s += FilePath;
    }
}

bool CMemDialog::OnInit() {
    if (_controls.isEmpty())
        BuildPresentation();
    size_t ramSize = (size_t)sizeof(size_t) << 29;
    const bool ramSize_defined = NWindows::NSystem::GetRamSize(ramSize);
    UInt32 ramSize_GB = (UInt32)(((UInt64)ramSize + (1u << 29)) >> 30);
    if (ramSize_GB == 0)
        ramSize_GB = 1;
    const bool is_Allowed = (!ramSize_defined || ramSize > ((UInt64)Required_GB << 30));
    _messageRam_GB = is_Allowed ? 0 : ramSize_GB;
    ReloadInfoMessage(is_Allowed ? NULL : &ramSize_GB);
    {
        UString s("GB");
        if (ramSize_defined) {
            s += " / ";
            s.Add_UInt32(ramSize_GB);
            s += " GB (RAM)";
        }
        SetItemText(IDT_MEM_GB, s);
    }
    const UINT valMin = 1;
    UINT valMax = 64;
    if (ramSize_defined) {
        const UINT k_max_val = 1u << 14;
        if (ramSize_GB >= k_max_val)
            valMax = k_max_val;
        else if (ramSize_GB > 1)
            valMax = (UINT)ramSize_GB - 1;
        else
            valMax = 1;
    }
    _spinMin = valMin;
    _spinMax = valMax;
    {
        UString s;
        s.Add_UInt32(Required_GB);
        SetItemText(IDE_MEM_SPIN_EDIT, s);
    }
    EnableSpin(false);
    const UINT buttonId = is_Allowed ? IDR_MEM_ACTION_ALLOW : IDR_MEM_ACTION_SKIP_ARC;
    static_cast<QRadioButton *>(_controls.value(buttonId))->setChecked(true);
    _controls.value(IDX_MEM_REMEMBER)->setVisible(ShowRemember);
    return true;
}

void CMemDialog::ReloadInfoMessage(const UInt32 *ramSize_GB) {
    UString s;
    if (ramSize_GB) {
        AddLangString(s, IDS_MEM_ERROR);
        s.Add_LF();
    }
    AddInfoMessage_To_String(s, ramSize_GB);
    if (!ArcPath.IsEmpty()) {
        s.Add_LF();
        AddLangString(s, TestMode ? IDS_PROGRESS_TESTING : IDS_PROGRESS_EXTRACTING);
        s += ": ";
        s += ArcPath;
    }
    SetItemText(IDT_MEM_MESSAGE, s);
}

void CMemDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && !_controls.isEmpty())
        ReloadInfoMessage(_messageRam_GB ? &_messageRam_GB : nullptr);
    QDialog::changeEvent(event);
}

void CMemDialog::OnContinue() {
    Remember = IsButtonCheckedBool(IDX_MEM_REMEMBER);
    NeedSave = IsButtonCheckedBool(IDX_MEM_SAVE_LIMIT);
    SkipArc = IsButtonCheckedBool(IDR_MEM_ACTION_SKIP_ARC);
    if (NeedSave) {
        UString s;
        GetItemText(IDE_MEM_SPIN_EDIT, s);
        const wchar_t *end;
        const UInt32 val = ConvertStringToUInt32(s.Ptr(), &end);
        if (s.IsEmpty() || *end != 0 || val > (1u << 30)) {
            QMessageBox::critical(
                this, windowTitle(),
                QString::fromWCharArray(NWindows::NError::MyFormatMessage(E_INVALIDARG)));
            return;
        }
        Limit_GB = val;
    }
    done(IDCONTINUE);
}
