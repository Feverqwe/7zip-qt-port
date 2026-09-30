// Adapted from 7-Zip OverwriteDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "OverwriteDialog.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "Common/IntToString.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "Windows/PropVariantConv.h"
#include <QFileIconProvider>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace NWindows;
static const unsigned kCurrentFileNameSizeLimit = 72;

void COverwriteDialog::ReduceString(UString &s) {
    const unsigned size = kCurrentFileNameSizeLimit;
    if (s.Len() > size) {
        s.Delete(size / 2, s.Len() - size);
        s.Insert(size / 2, L" ... ");
    }
    if (!s.IsEmpty() && s.Back() == ' ') {
        s.InsertAtFront(L'\"');
        s.Add_Char('\"');
    }
}

void AddSizeValue(UString &s, UInt64 value);
void AddSizeValue(UString &s, UInt64 value) {
    {
        wchar_t sz[32];
        ConvertUInt64ToString(value, sz);
        s += MyFormatNew(IDS_FILE_SIZE, sz);
    }
    if (value >= (1 << 10)) {
        char c;
        if (value >= ((UInt64)10 << 30)) {
            value >>= 30;
            c = 'G';
        } else if (value >= (10 << 20)) {
            value >>= 20;
            c = 'M';
        } else {
            value >>= 10;
            c = 'K';
        }
        s += " : ";
        s.Add_UInt64(value);
        s.Add_Space();
        s.Add_Char(c);
        s += "iB";
    }
}

void COverwriteDialog::SetFileInfoControl(const NOverwriteDialog::CFileInfo &fileInfo,
                                          unsigned textID, unsigned iconID, unsigned iconID_2) {
    {
        const UString &path = fileInfo.Path;
        const int slashPos = path.ReverseFind_PathSepar();
        UString s = path.Left((unsigned)(slashPos + 1));
        ReduceString(s);
        s.Add_LF();
        {
            UString s2 = path.Ptr((unsigned)(slashPos + 1));
            ReduceString(s2);
            s += s2;
        }
        s.Add_LF();
        if (fileInfo.Size_IsDefined)
            AddSizeValue(s, fileInfo.Size);
        s.Add_LF();
        if (fileInfo.Time_IsDefined) {
            AddLangString(s, IDS_PROP_MTIME);
            s += ": ";
            char t[64];
            ConvertUtcFileTimeToString(fileInfo.Time, t);
            s += t;
        }
        SetItemText(textID, s);
    }
    // Qt owns native file icons. Shell extraction of a second EXE/ICO icon
    // has no portable QFileIconProvider equivalent.
    QFileIconProvider provider;
    const auto path = QString::fromWCharArray(fileInfo.Path);
    const auto icon = fileInfo.Is_FileSystemFile ? provider.icon(QFileInfo(path))
                                                 : provider.icon(QFileIconProvider::File);
    static_cast<QLabel *>(_controls.value(iconID))->setPixmap(icon.pixmap(32, 32));
    _controls.value(iconID_2)->hide();
}

void COverwriteDialog::BuildPresentation() {
    setObjectName("overwriteDialog");
    setWindowTitle("Подтверждение замены файла");
    resize(700, 440);
    auto *layout = new QVBoxLayout(this);
    const auto label = [&](unsigned id, const char *name, const QString &text) {
        auto *control = new QLabel(text, this);
        control->setObjectName(name);
        control->setTextFormat(Qt::PlainText);
        control->setWordWrap(true);
        _controls.insert(id, control);
        return control;
    };
    layout->addWidget(label(IDT_OVERWRITE_HEADER, "overwriteHeader",
                            "Папка назначения уже содержит такой файл."));
    layout->addWidget(label(IDT_OVERWRITE_QUESTION_BEGIN, "overwriteQuestionBegin",
                            "Заменить существующий файл"));
    const auto fileRow = [&](unsigned textID, unsigned iconID, unsigned secondID,
                             const char *name) {
        auto *row = new QHBoxLayout;
        auto *icons = new QVBoxLayout;
        icons->addWidget(label(iconID, (QByteArray(name) + "Icon").constData(), {}));
        auto *second = label(secondID, (QByteArray(name) + "SecondIcon").constData(), {});
        second->hide();
        icons->addWidget(second);
        icons->addStretch();
        row->addLayout(icons);
        row->addWidget(label(textID, name, {}), 1);
        layout->addLayout(row);
    };
    fileRow(IDT_OVERWRITE_OLD_FILE_SIZE_TIME, IDI_OVERWRITE_OLD_FILE, IDI_OVERWRITE_OLD_FILE_2,
            "overwriteOldFile");
    layout->addWidget(label(IDT_OVERWRITE_QUESTION_END, "overwriteQuestionEnd", "этим файлом?"));
    fileRow(IDT_OVERWRITE_NEW_FILE_SIZE_TIME, IDI_OVERWRITE_NEW_FILE, IDI_OVERWRITE_NEW_FILE_2,
            "overwriteNewFile");
    layout->addStretch();
    auto *buttons = new QGridLayout;
    struct Button {
        unsigned Id;
        const char *Name;
        const char *Text;
        int Row;
        int Column;
    };
    for (const auto &spec : {Button{IDYES, "overwriteYes", "&Да", 0, 0},
                             Button{IDB_YES_TO_ALL, "overwriteYesAll", "Да для &всех", 0, 1},
                             Button{IDB_AUTO_RENAME, "overwriteRename", "&Переименовать", 0, 2},
                             Button{IDNO, "overwriteNo", "&Нет", 1, 0},
                             Button{IDB_NO_TO_ALL, "overwriteNoAll", "Нет для в&сех", 1, 1},
                             Button{IDCANCEL, "overwriteCancel", "&Отмена", 1, 2}}) {
        auto *button = new QPushButton(QString::fromUtf8(spec.Text), this);
        button->setObjectName(spec.Name);
        _controls.insert(spec.Id, button);
        buttons->addWidget(button, spec.Row, spec.Column);
        connect(button, &QPushButton::clicked, this,
                [this, id = spec.Id, button] { OnButtonClicked(id, button); });
    }
    layout->addLayout(buttons);
}

void COverwriteDialog::SetItemText(unsigned id, const UString &text) {
    static_cast<QLabel *>(_controls.value(id))->setText(QString::fromWCharArray(text));
}

bool COverwriteDialog::OnInit() {
    if (_controls.isEmpty())
        BuildPresentation();
    if (OldFileInfo.Is_Directory && NewFileInfo.Is_Directory) {
        static_cast<QLabel *>(_controls.value(IDT_OVERWRITE_HEADER))
            ->setText("Папка назначения уже содержит такую папку.");
        static_cast<QLabel *>(_controls.value(IDT_OVERWRITE_QUESTION_BEGIN))
            ->setText("Заменить существующую папку");
        static_cast<QLabel *>(_controls.value(IDT_OVERWRITE_QUESTION_END))->setText("этой папкой?");
    }
    SetFileInfoControl(OldFileInfo, IDT_OVERWRITE_OLD_FILE_SIZE_TIME, IDI_OVERWRITE_OLD_FILE,
                       IDI_OVERWRITE_OLD_FILE_2);
    SetFileInfoControl(NewFileInfo, IDT_OVERWRITE_NEW_FILE_SIZE_TIME, IDI_OVERWRITE_NEW_FILE,
                       IDI_OVERWRITE_NEW_FILE_2);
    if (!ShowExtraButtons) {
        _controls.value(IDB_YES_TO_ALL)->hide();
        _controls.value(IDB_NO_TO_ALL)->hide();
        _controls.value(IDB_AUTO_RENAME)->hide();
    }
    auto *button = static_cast<QPushButton *>(_controls.value(DefaultButton_is_NO ? IDNO : IDYES));
    button->setDefault(true);
    button->setFocus();
    return true;
}

bool COverwriteDialog::OnButtonClicked(unsigned buttonID, QWidget *) {
    switch (buttonID) {
    case IDYES:
    case IDNO:
    case IDB_YES_TO_ALL:
    case IDB_NO_TO_ALL:
    case IDB_AUTO_RENAME:
        done(int(buttonID));
        return true;
    case IDCANCEL:
        reject();
        return true;
    default:
        return false;
    }
}

INT_PTR COverwriteDialog::Create(QWidget *parent) {
    if (parent && parent != parentWidget())
        setParent(parent, windowFlags());
    OnInit();
    return exec();
}

void COverwriteDialog::reject() { done(IDCANCEL); }
