// Adapted from 7-Zip OverwriteDialog.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/FileManager/OverwriteDialogRes.h"
#include "Common/MyString.h"
#include "Common/MyWindows.h"
#include "qtmodalresult.h"
#include <QDialog>
#include <QHash>

namespace NOverwriteDialog {
struct CFileInfo {
    bool Size_IsDefined;
    bool Time_IsDefined;
    bool Is_FileSystemFile;
    UInt64 Size;
    FILETIME Time;
    UString Path;
    void SetTime(const FILETIME &t) {
        Time = t;
        Time_IsDefined = true;
    }
    void SetTime2(const FILETIME *t) {
        if (!t)
            Time_IsDefined = false;
        else
            SetTime(*t);
    }
    void SetSize(UInt64 size) {
        Size = size;
        Size_IsDefined = true;
    }
    void SetSize2(const UInt64 *size) {
        if (!size)
            Size_IsDefined = false;
        else
            SetSize(*size);
    }
    CFileInfo() : Size_IsDefined(false), Time_IsDefined(false), Is_FileSystemFile(false) {}
};
} // namespace NOverwriteDialog

class COverwriteDialog final : public QDialog {
    QHash<unsigned, QWidget *> _controls;
    void SetFileInfoControl(const NOverwriteDialog::CFileInfo &info, unsigned textID,
                            unsigned iconID, unsigned iconID_2);
    void SetItemText(unsigned id, const UString &text);
    void ReduceString(UString &text);
    bool OnButtonClicked(unsigned buttonID, QWidget *button);
    void BuildPresentation();

  public:
    bool ShowExtraButtons;
    bool DefaultButton_is_NO;
    NOverwriteDialog::CFileInfo OldFileInfo;
    NOverwriteDialog::CFileInfo NewFileInfo;
    explicit COverwriteDialog(QWidget *parent = nullptr)
        : QDialog(parent), ShowExtraButtons(true), DefaultButton_is_NO(false) {}
    bool OnInit();
    INT_PTR Create(QWidget *parent = nullptr);
    void reject() override;
};
