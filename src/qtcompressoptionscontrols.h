// Qt presentation services for the original COptionsDialog functional bodies.
#pragma once
#include "Common/MyString.h"
#include <QDialog>
#include <QHash>

class QComboBox;
class CQtCompressCombo {
    QComboBox *view_ = nullptr;

  public:
    void Attach(QWidget *widget);
    int GetCount() const;
    quint32 GetItemData_of_CurSel() const;
    int AddString_SetItemData(const UString &text, qintptr value);
    void ResetContent();
    void SetCurSel(int index);
};

class CQtCompressOptionsDialog : public QDialog {
    QHash<unsigned, QWidget *> controls_;
    bool initialized_ = false;
    void buildControls();

  protected:
    static constexpr unsigned CBN_SELCHANGE = 1;
    QWidget *GetItem(unsigned id) const;
    void CheckButton(unsigned id, bool value);
    bool IsButtonCheckedBool(unsigned id) const;
    void ShowItem_Bool(unsigned id, bool visible);
    void EnableItem(unsigned id, bool enabled);
    void SetItemText(unsigned id, const UString &text);
    void NormalizePosition();
    virtual bool OnInit() { return true; }
    virtual bool OnCommand(unsigned, unsigned, qintptr) { return false; }
    virtual bool OnButtonClicked(unsigned id, QWidget *);
    virtual void OnOK() { QDialog::accept(); }
    virtual void OnHelp() {}

  public:
    explicit CQtCompressOptionsDialog(QWidget *parent = nullptr);
    void setVisible(bool visible) override;
    void accept() override { OnOK(); }
};
