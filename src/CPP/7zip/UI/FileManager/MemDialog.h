// Adapted from 7-Zip MemDialog.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/FileManager/MemDialogRes.h"
#include "Common/MyString.h"
#include "Common/MyWindows.h"
#include "qtmodalresult.h"
#include <QDialog>
#include <QHash>

class CMemDialog final : public QDialog {
    QHash<unsigned, QWidget *> _controls;
    UInt32 _spinMin = 1;
    UInt32 _spinMax = 64;
    UInt32 _messageRam_GB = 0;
    void ReloadInfoMessage(const UInt32 *ramSize_GB);
    void changeEvent(QEvent *event) override;
    void BuildPresentation();
    void EnableSpin(bool enable);
    void SetItemText(unsigned id, const UString &text);
    void GetItemText(unsigned id, UString &text) const;
    bool IsButtonCheckedBool(unsigned id) const;
    void StepSpin(bool up);

  public:
    bool NeedSave;
    bool Remember;
    bool SkipArc;
    bool TestMode;
    bool ShowRemember;
    UInt32 Required_GB;
    UInt32 Limit_GB;
    UString ArcPath;
    UString FilePath;
    void AddInfoMessage_To_String(UString &s, const UInt32 *ramSize_GB = nullptr);
    explicit CMemDialog(QWidget *parent = nullptr)
        : QDialog(parent), NeedSave(false), Remember(false), SkipArc(false), TestMode(false),
          ShowRemember(true), Required_GB(4), Limit_GB(4) {}
    bool OnInit();
    void OnContinue();
    INT_PTR Create(QWidget *parent = nullptr);
    void reject() override;
};
